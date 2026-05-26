#pragma once

#include "CoreMinimal.h"
#include "UI/SBlockedDialog.h"
#include "UI/SForceUnlockDialog.h"
#include "UI/SUploadRetryDialog.h"

class FRcloneProcessManager;

namespace PoorforceDialogs
{
	EBlockedDialogResult ShowBlockedDialog(
		const FString& RelativePath,
		const FString& OwnerId,
		const FString& ElapsedText,
		bool bShowForceUnlock = true,
		bool bShowOpenAnyway = false);

	FForceUnlockDialogResult ShowForceUnlockDialog(
		const FString& RelativePath,
		const FString& OwnerId);

	EUploadRetryChoice ShowUploadRetryDialog(
		const FString& RelativePath,
		int32 ExitCode,
		const FString& Output);

	bool WaitForUploadsModal(TSharedPtr<FRcloneProcessManager> Rclone);
}
