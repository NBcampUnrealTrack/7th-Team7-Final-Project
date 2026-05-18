#pragma once

#include "CoreMinimal.h"

enum class EPoorforcePathMode : uint8
{
	LockOnly,
	LockAndSync,
};

struct FPoorforceManagedPath
{
	FString ContentPath;
	EPoorforcePathMode Mode = EPoorforcePathMode::LockOnly;
	FString RcloneRemote;
};

struct FPoorforceConfig
{
	FString UpstashUrl;
	FString UpstashToken;
	FString LockKeyNamespace;
	FString RcloneExecutable = TEXT("rclone");
	int32 LockOnlyTtlSeconds = 604800;     // 7d
	int32 LockAndSyncTtlSeconds = 259200;  // 3d
	TArray<FPoorforceManagedPath> ManagedPaths;
	FString DiscordWebhookUrl;

	bool IsValid() const
	{
		return !UpstashUrl.IsEmpty() && !UpstashToken.IsEmpty();
	}
};

namespace PoorforceConfigLoader
{
	bool LoadFromProjectRoot(FPoorforceConfig& OutConfig);

	FString GetExpectedConfigPath();
}
