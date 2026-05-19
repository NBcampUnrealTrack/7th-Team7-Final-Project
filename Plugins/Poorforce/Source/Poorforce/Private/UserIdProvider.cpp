#include "UserIdProvider.h"

#include "PoorforceLog.h"

#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"

namespace PoorforceUserId
{
	static FString CachedUserId;
	static bool bResolved = false;

	static bool TryRunGit(FString& OutEmail)
	{
		void* StdOutReadHandle = nullptr;
		void* StdOutWriteHandle = nullptr;
		if (!FPlatformProcess::CreatePipe(StdOutReadHandle, StdOutWriteHandle))
		{
			return false;
		}

		FProcHandle Proc = FPlatformProcess::CreateProc(
			TEXT("git"),
			TEXT("config user.email"),
			/*bLaunchDetached=*/ false,
			/*bLaunchHidden=*/ true,
			/*bLaunchReallyHidden=*/ true,
			/*OutProcessID=*/ nullptr,
			/*PriorityModifier=*/ 0,
			/*OptionalWorkingDirectory=*/ *FPaths::ProjectDir(),
			/*PipeWriteChild=*/ StdOutWriteHandle,
			/*PipeReadChild=*/ nullptr);

		if (!Proc.IsValid())
		{
			FPlatformProcess::ClosePipe(StdOutReadHandle, StdOutWriteHandle);
			return false;
		}

		FString Output;
		while (FPlatformProcess::IsProcRunning(Proc))
		{
			Output += FPlatformProcess::ReadPipe(StdOutReadHandle);
			FPlatformProcess::Sleep(0.01f);
		}
		Output += FPlatformProcess::ReadPipe(StdOutReadHandle);

		int32 ReturnCode = -1;
		FPlatformProcess::GetProcReturnCode(Proc, &ReturnCode);
		FPlatformProcess::CloseProc(Proc);
		FPlatformProcess::ClosePipe(StdOutReadHandle, StdOutWriteHandle);

		if (ReturnCode != 0) return false;

		Output.TrimStartAndEndInline();
		if (Output.IsEmpty()) return false;

		OutEmail = MoveTemp(Output);
		return true;
	}

	static FString MakeFallback()
	{
		const FString UserName = FPlatformProcess::UserName();
		const FString LoginId = FPlatformMisc::GetLoginId();
		return FString::Printf(TEXT("%s@%s"), *UserName, *LoginId);
	}

	const FString& Get()
	{
		if (bResolved) return CachedUserId;

		FString Email;
		if (TryRunGit(Email))
		{
			CachedUserId = MoveTemp(Email);
			UE_LOG(LogPoorforce, Log, TEXT("UserId resolved from git: %s"), *CachedUserId);
		}
		else
		{
			CachedUserId = MakeFallback();
			UE_LOG(LogPoorforce, Log, TEXT("UserId resolved from fallback: %s"), *CachedUserId);
		}

		bResolved = true;
		return CachedUserId;
	}

	void ResetForTest()
	{
		CachedUserId.Empty();
		bResolved = false;
	}
}
