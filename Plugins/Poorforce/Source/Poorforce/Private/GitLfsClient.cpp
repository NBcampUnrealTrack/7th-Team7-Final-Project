#include "GitLfsClient.h"

#include "PoorforceLog.h"

#include "Containers/Ticker.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

namespace PoorforceGitLfs
{
	namespace
	{
		struct FActiveLock
		{
			FProcHandle Handle;
			void* ReadPipe = nullptr;
			void* WritePipe = nullptr;
			FString AccumulatedOutput;
			FOnLockComplete OnComplete;
			FString RelativeFilePath;
		};

		ELockResult Classify(int32 ExitCode, const FString& Output)
		{
			if (ExitCode == 0) return ELockResult::Success;

			const bool bAlreadyMine =
				Output.Contains(TEXT("already locked by you"), ESearchCase::IgnoreCase) ||
				Output.Contains(TEXT("Already created lock"), ESearchCase::IgnoreCase);
			if (bAlreadyMine) return ELockResult::AlreadyOwnedByMe;

			const bool bOtherOwner =
				Output.Contains(TEXT("already created by"), ESearchCase::IgnoreCase) ||
				Output.Contains(TEXT("is already locked"), ESearchCase::IgnoreCase) ||
				Output.Contains(TEXT("lock already created by"), ESearchCase::IgnoreCase);
			if (bOtherOwner) return ELockResult::OwnedByOther;

			const bool bSetup =
				Output.Contains(TEXT("not a git repository"), ESearchCase::IgnoreCase) ||
				Output.Contains(TEXT("git-lfs"), ESearchCase::IgnoreCase) && Output.Contains(TEXT("not found"), ESearchCase::IgnoreCase) ||
				Output.Contains(TEXT("Remote \""), ESearchCase::IgnoreCase) ||
				Output.Contains(TEXT("not lockable"), ESearchCase::IgnoreCase);
			if (bSetup) return ELockResult::SetupError;

			return ELockResult::Other;
		}
	}

	void TryUnlock(const FString& RelativeFilePath)
	{
		if (RelativeFilePath.IsEmpty()) return;

		void* ReadPipe = nullptr;
		void* WritePipe = nullptr;
		if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
		{
			UE_LOG(LogPoorforce, Warning, TEXT("LFS unlock: pipe failed for %s"), *RelativeFilePath);
			return;
		}

		const FString Args = FString::Printf(TEXT("lfs unlock \"%s\""), *RelativeFilePath);

		FProcHandle Handle = FPlatformProcess::CreateProc(
			TEXT("git"),
			*Args,
			/*bLaunchDetached=*/ false,
			/*bLaunchHidden=*/ true,
			/*bLaunchReallyHidden=*/ true,
			/*OutProcessID=*/ nullptr,
			/*PriorityModifier=*/ 0,
			/*OptionalWorkingDirectory=*/ *FPaths::ProjectDir(),
			/*PipeWriteChild=*/ WritePipe,
			/*PipeReadChild=*/ nullptr);

		if (!Handle.IsValid())
		{
			FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
			UE_LOG(LogPoorforce, Warning, TEXT("LFS unlock: spawn failed for %s"), *RelativeFilePath);
			return;
		}

		struct FActiveUnlock
		{
			FProcHandle Handle;
			void* ReadPipe;
			void* WritePipe;
			FString Output;
			FString Path;
		};
		TSharedPtr<FActiveUnlock> Proc = MakeShared<FActiveUnlock>();
		Proc->Handle = Handle;
		Proc->ReadPipe = ReadPipe;
		Proc->WritePipe = WritePipe;
		Proc->Path = RelativeFilePath;

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[Proc](float) -> bool
				{
					Proc->Output += FPlatformProcess::ReadPipe(Proc->ReadPipe);
					if (FPlatformProcess::IsProcRunning(Proc->Handle)) return true;
					Proc->Output += FPlatformProcess::ReadPipe(Proc->ReadPipe);

					int32 ExitCode = -1;
					FPlatformProcess::GetProcReturnCode(Proc->Handle, &ExitCode);

					if (ExitCode == 0)
					{
						UE_LOG(LogPoorforce, Log, TEXT("LFS unlock done: %s"), *Proc->Path);
					}
					else
					{
						UE_LOG(LogPoorforce, Verbose, TEXT("LFS unlock failed (exit=%d) %s\n%s"),
							ExitCode, *Proc->Path, *Proc->Output);
					}

					FPlatformProcess::CloseProc(Proc->Handle);
					FPlatformProcess::ClosePipe(Proc->ReadPipe, Proc->WritePipe);
					return false;
				}),
			0.1f);
	}

	void TryLock(const FString& RelativeFilePath, FOnLockComplete OnComplete)
	{
		if (RelativeFilePath.IsEmpty())
		{
			FLockOutcome Out;
			Out.Result = ELockResult::Other;
			OnComplete(Out);
			return;
		}

		void* ReadPipe = nullptr;
		void* WritePipe = nullptr;
		if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
		{
			FLockOutcome Out;
			Out.Result = ELockResult::Other;
			OnComplete(Out);
			return;
		}

		const FString Args = FString::Printf(TEXT("lfs lock \"%s\""), *RelativeFilePath);

		FProcHandle Handle = FPlatformProcess::CreateProc(
			TEXT("git"),
			*Args,
			/*bLaunchDetached=*/ false,
			/*bLaunchHidden=*/ true,
			/*bLaunchReallyHidden=*/ true,
			/*OutProcessID=*/ nullptr,
			/*PriorityModifier=*/ 0,
			/*OptionalWorkingDirectory=*/ *FPaths::ProjectDir(),
			/*PipeWriteChild=*/ WritePipe,
			/*PipeReadChild=*/ nullptr);

		if (!Handle.IsValid())
		{
			FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
			FLockOutcome Out;
			Out.Result = ELockResult::SetupError;
			Out.Stderr = TEXT("Failed to spawn 'git lfs lock' process");
			OnComplete(Out);
			return;
		}

		TSharedPtr<FActiveLock> Proc = MakeShared<FActiveLock>();
		Proc->Handle = Handle;
		Proc->ReadPipe = ReadPipe;
		Proc->WritePipe = WritePipe;
		Proc->OnComplete = MoveTemp(OnComplete);
		Proc->RelativeFilePath = RelativeFilePath;

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[Proc](float) -> bool
				{
					Proc->AccumulatedOutput += FPlatformProcess::ReadPipe(Proc->ReadPipe);

					if (FPlatformProcess::IsProcRunning(Proc->Handle))
					{
						return true;
					}

					Proc->AccumulatedOutput += FPlatformProcess::ReadPipe(Proc->ReadPipe);

					int32 ExitCode = -1;
					FPlatformProcess::GetProcReturnCode(Proc->Handle, &ExitCode);

					FLockOutcome Out;
					Out.ExitCode = ExitCode;
					Out.Stderr = Proc->AccumulatedOutput;
					Out.Result = Classify(ExitCode, Proc->AccumulatedOutput);

					FPlatformProcess::CloseProc(Proc->Handle);
					FPlatformProcess::ClosePipe(Proc->ReadPipe, Proc->WritePipe);

					if (Proc->OnComplete)
					{
						Proc->OnComplete(Out);
					}

					return false;
				}),
			0.1f);
	}
}
