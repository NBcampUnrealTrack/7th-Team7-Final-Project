#include "PoorforcePathResolver.h"

#include "PoorforceConfig.h"

#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "UObject/Object.h"

namespace PoorforcePathResolver
{
	const FPoorforceManagedPath* ResolveLongestPrefix(
		const FString& PackageName,
		const TArray<FPoorforceManagedPath>& Paths)
	{
		const FPoorforceManagedPath* LongestMatch = nullptr;
		int32 LongestLen = 0;

		for (const FPoorforceManagedPath& Candidate : Paths)
		{
			if (Candidate.ContentPath.IsEmpty()) continue;
			if (!PackageName.StartsWith(Candidate.ContentPath, ESearchCase::CaseSensitive)) continue;

			const int32 Len = Candidate.ContentPath.Len();
			if (Len > LongestLen)
			{
				LongestMatch = &Candidate;
				LongestLen = Len;
			}
		}

		return LongestMatch;
	}

	FString MakeRelativePath(const FString& PackageName, const FPoorforceManagedPath& Match)
	{
		if (!PackageName.StartsWith(Match.ContentPath, ESearchCase::CaseSensitive))
		{
			return FString{};
		}

		return PackageName.RightChop(Match.ContentPath.Len());
	}

	FString MakeLockKey(const FString& Namespace, const FString& RelativePath)
	{
		if (Namespace.IsEmpty())
		{
			return FString::Printf(TEXT("asset:%s"), *RelativePath);
		}

		return FString::Printf(TEXT("%s:asset:%s"), *Namespace, *RelativePath);
	}

	FString ExtractRelativePathFromKey(const FString& LockKey)
	{
		const FString Marker = TEXT(":asset:");
		const int32 Idx = LockKey.Find(Marker, ESearchCase::CaseSensitive);
		if (Idx != INDEX_NONE)
		{
			return LockKey.RightChop(Idx + Marker.Len());
		}

		const FString BareMarker = TEXT("asset:");
		if (LockKey.StartsWith(BareMarker, ESearchCase::CaseSensitive))
		{
			return LockKey.RightChop(BareMarker.Len());
		}

		return LockKey;
	}

	bool ReconstructLockOnlyGitPath(
		const FString& RelativePath,
		const TArray<FPoorforceManagedPath>& Paths,
		FString& OutGitPath)
	{
		if (RelativePath.IsEmpty()) return false;

		const FString Extensions[] =
		{
			FPackageName::GetAssetPackageExtension(),   // .uasset
			FPackageName::GetMapPackageExtension()       // .umap
		};

		const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

		for (const FPoorforceManagedPath& Managed : Paths)
		{
			if (Managed.Mode != EPoorforcePathMode::LockOnly) continue;
			if (Managed.ContentPath.IsEmpty()) continue;

			const FString PackageName = Managed.ContentPath + RelativePath;

			for (const FString& Extension : Extensions)
			{
				FString LocalFile;
				if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, LocalFile, Extension))
				{
					continue;
				}

				if (!FPaths::FileExists(LocalFile)) continue;

				FString GitPath = FPaths::ConvertRelativePathToFull(LocalFile);
				if (GitPath.StartsWith(ProjectDir, ESearchCase::IgnoreCase))
				{
					GitPath = GitPath.RightChop(ProjectDir.Len());
				}
				GitPath.ReplaceInline(TEXT("\\"), TEXT("/"));
				while (GitPath.StartsWith(TEXT("/"))) GitPath.RightChopInline(1);

				OutGitPath = GitPath;
				return true;
			}
		}

		return false;
	}

	FString GetPackageExtensionFor(const UObject* Asset)
	{
		if (IsValid(Asset) && Asset->IsA<UWorld>())
		{
			return FPackageName::GetMapPackageExtension();
		}

		return FPackageName::GetAssetPackageExtension();
	}

	bool MakeLocalFilePath(const FString& PackageName, const UObject* Asset, FString& OutLocalPath)
	{
		const FString Extension = GetPackageExtensionFor(Asset);
		return FPackageName::TryConvertLongPackageNameToFilename(PackageName, OutLocalPath, Extension);
	}

	FString MakeRemoteFilePath(const FString& RcloneRemote, const FString& RelativePath, const FString& Extension)
	{
		FString Trimmed = RcloneRemote;
		while (Trimmed.EndsWith(TEXT("/")))
		{
			Trimmed.LeftChopInline(1);
		}

		return FString::Printf(TEXT("%s/%s%s"), *Trimmed, *RelativePath, *Extension);
	}

	TArray<FSidecarPair> MakeSidecarPaths(
		const FString& PackageName,
		const UObject* Asset,
		const FString& RcloneRemote,
		const FString& RelativePath)
	{
		TArray<FSidecarPair> Out;

		if (!IsValid(Asset) || !Asset->IsA<UWorld>())
		{
			return Out;
		}

		const FString BuiltDataSuffix      = TEXT("_BuiltData");
		const FString BuiltDataPackageName = PackageName + BuiltDataSuffix;
		const FString BuiltDataRelative    = RelativePath + BuiltDataSuffix;
		const FString AssetExt             = FPackageName::GetAssetPackageExtension();

		FString BuiltDataLocal;
		if (!FPackageName::TryConvertLongPackageNameToFilename(BuiltDataPackageName, BuiltDataLocal, AssetExt))
		{
			return Out;
		}

		FSidecarPair Pair;
		Pair.LocalPath  = BuiltDataLocal;
		Pair.RemotePath = MakeRemoteFilePath(RcloneRemote, BuiltDataRelative, AssetExt);
		Out.Add(MoveTemp(Pair));

		return Out;
	}
}
