#pragma once

#include "CoreMinimal.h"

class UObject;
struct FPoorforceManagedPath;

namespace PoorforcePathResolver
{
	const FPoorforceManagedPath* ResolveLongestPrefix(
		const FString& PackageName,
		const TArray<FPoorforceManagedPath>& Paths);

	FString MakeRelativePath(const FString& PackageName, const FPoorforceManagedPath& Match);

	FString MakeLockKey(const FString& Namespace, const FString& RelativePath);

	FString GetPackageExtensionFor(const UObject* Asset);

	bool MakeLocalFilePath(const FString& PackageName, const UObject* Asset, FString& OutLocalPath);

	FString MakeRemoteFilePath(const FString& RcloneRemote, const FString& RelativePath, const FString& Extension);

	struct FSidecarPair
	{
		FString LocalPath;
		FString RemotePath;
	};

	TArray<FSidecarPair> MakeSidecarPaths(
		const FString& PackageName,
		const UObject* Asset,
		const FString& RcloneRemote,
		const FString& RelativePath);
}
