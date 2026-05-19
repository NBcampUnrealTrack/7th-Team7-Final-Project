#pragma once

#include "CoreMinimal.h"

namespace PoorforceDiscord
{
	void SendForceUnlockNotice(
		const FString& WebhookUrl,
		const FString& RelativePath,
		const FString& OriginalOwner,
		const FString& ForceUnlockedBy,
		const FString& Reason);
}
