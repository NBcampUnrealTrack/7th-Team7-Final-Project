#include "RcloneProcessManager.h"

#include "PoorforceLog.h"

#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

namespace
{
	constexpr float TickIntervalSeconds = 0.1f;

	FString QuoteIfNeeded(const FString& In)
	{
		if (In.Contains(TEXT(" ")) && !In.StartsWith(TEXT("\"")))
		{
			return FString::Printf(TEXT("\"%s\""), *In);
		}
		return In;
	}
}

FRcloneProcessManager::FRcloneProcessManager(FString InExecutable)
	: Executable(MoveTemp(InExecutable))
{
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FRcloneProcessManager::Tick),
		TickIntervalSeconds);
}

FRcloneProcessManager::~FRcloneProcessManager()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	CancelAll();
}

void FRcloneProcessManager::StartCopyTo(
	EDirection Direction,
	const FString& LocalPath,
	const FString& RemotePath,
	FOnComplete OnComplete)
{
	const FString Source = (Direction == EDirection::Download) ? RemotePath : LocalPath;
	const FString Dest   = (Direction == EDirection::Download) ? LocalPath : RemotePath;

	// --checksum: source/dest 의 size+checksum 같으면 skip (다운로드/업로드 자체 안 함)
	const FString Args = FString::Printf(TEXT("copyto --checksum %s %s -v"),
		*QuoteIfNeeded(Source), *QuoteIfNeeded(Dest));

	const TCHAR* DirText = (Direction == EDirection::Download) ? TEXT("download") : TEXT("upload");
	const FString Description = FString::Printf(TEXT("%s: %s -> %s"), DirText, *Source, *Dest);

	TUniquePtr<FActiveProcess> Proc = MakeUnique<FActiveProcess>();
	Proc->OnComplete = MoveTemp(OnComplete);
	Proc->Description = Description;

	if (!FPlatformProcess::CreatePipe(Proc->ReadPipe, Proc->WritePipe))
	{
		UE_LOG(LogPoorforce, Warning, TEXT("rclone: failed to create pipe (%s)"), *Description);
		Proc->OnComplete(false, -1, FString{});
		return;
	}

	Proc->Handle = FPlatformProcess::CreateProc(
		*Executable,
		*Args,
		/*bLaunchDetached=*/ false,
		/*bLaunchHidden=*/ true,
		/*bLaunchReallyHidden=*/ true,
		/*OutProcessID=*/ nullptr,
		/*PriorityModifier=*/ 0,
		/*OptionalWorkingDirectory=*/ *FPaths::ProjectDir(),
		/*PipeWriteChild=*/ Proc->WritePipe,
		/*PipeReadChild=*/ nullptr);

	if (!Proc->Handle.IsValid())
	{
		UE_LOG(LogPoorforce, Warning, TEXT("rclone: failed to spawn process '%s %s'"), *Executable, *Args);
		FPlatformProcess::ClosePipe(Proc->ReadPipe, Proc->WritePipe);
		Proc->OnComplete(false, -1, FString{});
		return;
	}

	UE_LOG(LogPoorforce, Log, TEXT("rclone: started [%s]"), *Description);

	Active.Add(MoveTemp(Proc));
}

void FRcloneProcessManager::StartCheck(
	const FString& LocalPath,
	const FString& RemotePath,
	FOnComplete OnComplete)
{
	// rclone check 는 폴더 단위 비교라 단일 파일은 부모 폴더 + --include 로 우회.
	// exit 0 = 같음, 1 = 다름.
	const FString LocalDir = FPaths::GetPath(LocalPath);

	int32 LastSlash = INDEX_NONE;
	RemotePath.FindLastChar(TEXT('/'), LastSlash);
	const FString RemoteDir = (LastSlash != INDEX_NONE) ? RemotePath.Left(LastSlash) : RemotePath;

	const FString Filename = FPaths::GetCleanFilename(LocalPath);

	const FString Args = FString::Printf(TEXT("check %s %s --include \"%s\" --max-depth 1 -v"),
		*QuoteIfNeeded(LocalDir), *QuoteIfNeeded(RemoteDir), *Filename);

	const FString Description = FString::Printf(TEXT("check: %s vs %s"), *LocalPath, *RemotePath);

	TUniquePtr<FActiveProcess> Proc = MakeUnique<FActiveProcess>();
	Proc->OnComplete = MoveTemp(OnComplete);
	Proc->Description = Description;

	if (!FPlatformProcess::CreatePipe(Proc->ReadPipe, Proc->WritePipe))
	{
		UE_LOG(LogPoorforce, Warning, TEXT("rclone: failed to create pipe (%s)"), *Description);
		Proc->OnComplete(false, -1, FString{});
		return;
	}

	Proc->Handle = FPlatformProcess::CreateProc(
		*Executable,
		*Args,
		/*bLaunchDetached=*/ false,
		/*bLaunchHidden=*/ true,
		/*bLaunchReallyHidden=*/ true,
		/*OutProcessID=*/ nullptr,
		/*PriorityModifier=*/ 0,
		/*OptionalWorkingDirectory=*/ *FPaths::ProjectDir(),
		/*PipeWriteChild=*/ Proc->WritePipe,
		/*PipeReadChild=*/ nullptr);

	if (!Proc->Handle.IsValid())
	{
		UE_LOG(LogPoorforce, Warning, TEXT("rclone: failed to spawn process '%s %s'"), *Executable, *Args);
		FPlatformProcess::ClosePipe(Proc->ReadPipe, Proc->WritePipe);
		Proc->OnComplete(false, -1, FString{});
		return;
	}

	UE_LOG(LogPoorforce, Log, TEXT("rclone: started [%s]"), *Description);

	Active.Add(MoveTemp(Proc));
}

bool FRcloneProcessManager::Tick(float DeltaTime)
{
	for (int32 i = Active.Num() - 1; i >= 0; --i)
	{
		FActiveProcess& Proc = *Active[i];

		Proc.AccumulatedOutput += FPlatformProcess::ReadPipe(Proc.ReadPipe);

		if (FPlatformProcess::IsProcRunning(Proc.Handle))
		{
			continue;
		}

		Proc.AccumulatedOutput += FPlatformProcess::ReadPipe(Proc.ReadPipe);

		int32 ExitCode = -1;
		FPlatformProcess::GetProcReturnCode(Proc.Handle, &ExitCode);

		const bool bSuccess = (ExitCode == 0);

		TUniquePtr<FActiveProcess> Detached = MoveTemp(Active[i]);
		Active.RemoveAt(i);

		FinalizeProcess(*Detached, bSuccess, ExitCode);
	}

	return true;
}

void FRcloneProcessManager::FinalizeProcess(FActiveProcess& Proc, bool bSuccess, int32 ExitCode)
{
	if (bSuccess)
	{
		// rclone -v 출력에서 "0 B / 0 B" 면 실제 transfer 없이 skip (--checksum 비교 결과 동일).
		const bool bNothingTransferred =
			Proc.AccumulatedOutput.Contains(TEXT("0 B / 0 B"), ESearchCase::IgnoreCase);

		if (bNothingTransferred)
		{
			UE_LOG(LogPoorforce, Log, TEXT("rclone: unchanged, skipped [%s]"), *Proc.Description);
		}
		else
		{
			UE_LOG(LogPoorforce, Log, TEXT("rclone: transferred [%s]"), *Proc.Description);
		}
	}
	else
	{
		UE_LOG(LogPoorforce, Warning, TEXT("rclone: failed (exit=%d) [%s]\nOutput: %s"),
			ExitCode, *Proc.Description, *Proc.AccumulatedOutput);
	}

	FPlatformProcess::CloseProc(Proc.Handle);
	FPlatformProcess::ClosePipe(Proc.ReadPipe, Proc.WritePipe);
	Proc.ReadPipe = nullptr;
	Proc.WritePipe = nullptr;

	if (Proc.OnComplete)
	{
		Proc.OnComplete(bSuccess, ExitCode, Proc.AccumulatedOutput);
	}
}

TArray<FString> FRcloneProcessManager::GetActiveDescriptions() const
{
	TArray<FString> Out;
	Out.Reserve(Active.Num());
	for (const TUniquePtr<FActiveProcess>& Proc : Active)
	{
		Out.Add(Proc->Description);
	}
	return Out;
}

void FRcloneProcessManager::CancelAll()
{
	for (TUniquePtr<FActiveProcess>& Proc : Active)
	{
		if (Proc->Handle.IsValid() && FPlatformProcess::IsProcRunning(Proc->Handle))
		{
			FPlatformProcess::TerminateProc(Proc->Handle, /*KillTree=*/ true);
		}

		FinalizeProcess(*Proc, false, -2);
	}

	Active.Empty();
}
