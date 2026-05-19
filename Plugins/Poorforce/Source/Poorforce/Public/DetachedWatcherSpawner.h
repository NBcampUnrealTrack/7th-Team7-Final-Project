#pragma once

#include "CoreMinimal.h"

class FDetachedWatcherSpawner
{
public:
	FDetachedWatcherSpawner(
		FString InRcloneExecutable,
		FString InUpstashUrl,
		FString InUpstashToken,
		FString InUserId);

	~FDetachedWatcherSpawner();

	bool SpawnWatcher(
		const FString& LockKey,
		const FString& LocalFilePath,
		const FString& RemoteFilePath);

	void SignalWatcherExit(const FString& LockKey);

	bool IsWatcherActive(const FString& LockKey) const;

	void CleanupStaleArtifacts();

private:
	struct FActiveWatcher
	{
		FString ScriptPath;
		FString SentinelPath;
	};

	FString RcloneExecutable;
	FString UpstashUrl;
	FString UpstashToken;
	FString UserId;

	TMap<FString, FActiveWatcher> Active;

	FString GetScriptDir() const;
	FString MakeSafeFilename(const FString& LockKey) const;
	bool EnsureScriptDirExists() const;
};
