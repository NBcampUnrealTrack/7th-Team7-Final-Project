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

	// 락 키(<ns>:asset:<rel> 또는 asset:<rel>)에서 RelativePath 만 뽑는다.
	FString ExtractRelativePathFromKey(const FString& LockKey);

	// RelativePath + LockOnly ManagedPaths 로 디스크의 .uasset/.umap 을 찾아
	// 리포 루트 기준 git 상대경로를 복원한다 (LFS unlock 인자용). 실패 시 false.
	bool ReconstructLockOnlyGitPath(
		const FString& RelativePath,
		const TArray<FPoorforceManagedPath>& Paths,
		FString& OutGitPath);

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
