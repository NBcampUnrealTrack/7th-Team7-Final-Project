#pragma once

#include "CoreMinimal.h"

namespace PoorforceGitLfs
{
	enum class ELockResult : uint8
	{
		Success,            // 락 획득
		AlreadyOwnedByMe,   // 이미 내가 가진 락 (정상)
		OwnedByOther,       // 다른 사람이 가진 락
		SetupError,         // git/lfs 미설치, .gitattributes 미설정 등
		Other,              // 그 외 (네트워크 등)
	};

	struct FLockOutcome
	{
		ELockResult Result = ELockResult::Other;
		int32 ExitCode = -1;
		FString Stderr;
	};

	using FOnLockComplete = TFunction<void(const FLockOutcome& Outcome)>;

	void TryLock(const FString& RelativeFilePath, FOnLockComplete OnComplete);

	// fire-and-forget. 결과는 로그로만.
	void TryUnlock(const FString& RelativeFilePath);
}
