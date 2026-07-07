#include "ItemEditor/GYItemEditorSaveLock.h"

#include "GitLfsClient.h"
#include "LockServerClient.h"
#include "Poorforce.h"
#include "PoorforceConfig.h"
#include "PoorforcePathResolver.h"
#include "UserIdProvider.h"

#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogGYItemEditorLock, Log, All);

#define LOCTEXT_NAMESPACE "GYItemEditor"

namespace
{
	void PumpUntil(bool& bDone)
	{
		while (!bDone)
		{
			FSlateApplication::Get().Tick();
			FTSTicker::GetCoreTicker().Tick(0.033f);
			FPlatformProcess::Sleep(0.01f);
		}
	}

	struct FResolvedTarget
	{
		FString PackageName;
		FString RelativePath;
		FString LockKey;
		FString GitPath;   // LockOnly 경로만. 비어 있으면 LFS 락 생략
		int32 TtlSeconds = 0;
	};

	enum class ELfsOwnership : uint8
	{
		Mine,
		Theirs,
		Unknown,
	};

	ELfsOwnership QueryLfsOwnership(const FString& GitPath, FString& OutOwnerName)
	{
		int32 ExitCode = -1;
		FString StdOut;
		FString StdErr;
		FPlatformProcess::ExecProcess(TEXT("git"), TEXT("lfs locks --verify --json"),
			&ExitCode, &StdOut, &StdErr, *FPaths::ProjectDir());
		if (ExitCode != 0) return ELfsOwnership::Unknown;

		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(StdOut), Root) || !Root.IsValid())
		{
			return ELfsOwnership::Unknown;
		}

		auto ContainsPath = [&GitPath](const TArray<TSharedPtr<FJsonValue>>* Locks, FString* OutOwner) -> bool
		{
			if (Locks == nullptr) return false;

			for (const TSharedPtr<FJsonValue>& LockValue : *Locks)
			{
				const TSharedPtr<FJsonObject>* LockObject = nullptr;
				if (!LockValue.IsValid() || !LockValue->TryGetObject(LockObject)) continue;

				FString Path;
				if (!(*LockObject)->TryGetStringField(TEXT("path"), Path)) continue;
				if (!Path.Equals(GitPath, ESearchCase::IgnoreCase)) continue;

				if (OutOwner != nullptr)
				{
					const TSharedPtr<FJsonObject>* Owner = nullptr;
					if ((*LockObject)->TryGetObjectField(TEXT("owner"), Owner))
					{
						(*Owner)->TryGetStringField(TEXT("name"), *OutOwner);
					}
				}
				return true;
			}
			return false;
		};

		const TArray<TSharedPtr<FJsonValue>>* Ours = nullptr;
		Root->TryGetArrayField(TEXT("ours"), Ours);
		if (ContainsPath(Ours, nullptr)) return ELfsOwnership::Mine;

		const TArray<TSharedPtr<FJsonValue>>* Theirs = nullptr;
		Root->TryGetArrayField(TEXT("theirs"), Theirs);
		if (ContainsPath(Theirs, &OutOwnerName)) return ELfsOwnership::Theirs;

		return ELfsOwnership::Unknown;
	}
}

GYItemEditorSaveLock::FAcquireResult GYItemEditorSaveLock::TryAcquireForPackages(const TArray<UPackage*>& Packages)
{
	FAcquireResult Result;

	if (!FModuleManager::Get().IsModuleLoaded("Poorforce")) return Result;

	FPoorforceModule& Poorforce = FModuleManager::GetModuleChecked<FPoorforceModule>("Poorforce");
	if (!Poorforce.IsEnabled()) return Result;

	TSharedPtr<FLockServerClient> Client = Poorforce.GetLockClient();
	if (!Client.IsValid()) return Result;

	const FPoorforceConfig& Config = Poorforce.GetConfig();
	const FString UserId = PoorforceUserId::Get();

	TArray<FResolvedTarget> Targets;
	for (const UPackage* Package : Packages)
	{
		if (Package == nullptr) continue;

		const FString PackageName = Package->GetName();
		const FPoorforceManagedPath* Match =
			PoorforcePathResolver::ResolveLongestPrefix(PackageName, Config.ManagedPaths);
		if (Match == nullptr) continue;

		FResolvedTarget Target;
		Target.PackageName = PackageName;
		Target.RelativePath = PoorforcePathResolver::MakeRelativePath(PackageName, *Match);
		Target.LockKey = PoorforcePathResolver::MakeLockKey(Config.LockKeyNamespace, Target.RelativePath);
		Target.TtlSeconds = Match->Mode == EPoorforcePathMode::LockAndSync
			? Config.LockAndSyncTtlSeconds
			: Config.LockOnlyTtlSeconds;

		if (Match->Mode == EPoorforcePathMode::LockOnly)
		{
			PoorforcePathResolver::ReconstructLockOnlyGitPath(Target.RelativePath, Config.ManagedPaths, Target.GitPath);
		}

		Targets.Add(MoveTemp(Target));
	}

	if (Targets.Num() == 0) return Result;

	FScopedSlowTask SlowTask(static_cast<float>(Targets.Num()), LOCTEXT("AcquiringSaveLocks", "저장 락 확인 중..."));
	SlowTask.MakeDialog();

	TArray<FString> NewRedisKeys;
	TArray<FString> NewLfsPaths;

	for (const FResolvedTarget& Target : Targets)
	{
		SlowTask.EnterProgressFrame(1.f, FText::FromString(Target.RelativePath));

		bool bAcquireDone = false;
		PoorforceLock::EAcquireResult AcquireResult = PoorforceLock::EAcquireResult::NetworkError;
		Client->TryAcquire(Target.LockKey, UserId, Target.TtlSeconds,
			[&AcquireResult, &bAcquireDone](PoorforceLock::EAcquireResult InResult)
			{
				AcquireResult = InResult;
				bAcquireDone = true;
			});
		PumpUntil(bAcquireDone);

		if (AcquireResult == PoorforceLock::EAcquireResult::Acquired)
		{
			NewRedisKeys.Add(Target.LockKey);
			UE_LOG(LogGYItemEditorLock, Log, TEXT("Lock acquired: %s"), *Target.RelativePath);
		}
		else if (AcquireResult == PoorforceLock::EAcquireResult::AlreadyHeld)
		{
			bool bGetDone = false;
			TOptional<PoorforceLock::FLockEntry> Entry;
			Client->Get(Target.LockKey,
				[&Entry, &bGetDone](bool bExists, const TOptional<PoorforceLock::FLockEntry>& InEntry)
				{
					if (bExists) Entry = InEntry;
					bGetDone = true;
				});
			PumpUntil(bGetDone);

			const bool bMine = Entry.IsSet() && Entry->OwnerId.Equals(UserId, ESearchCase::CaseSensitive);
			if (!bMine)
			{
				Result.bSuccess = false;
				Result.Blocked.Add({ Target.PackageName,
					FString::Printf(TEXT("락 보유자: %s"), Entry.IsSet() ? *Entry->OwnerId : TEXT("알 수 없음")) });
				break;
			}

			// 내 락 재진입 — TTL만 연장
			Client->Refresh(Target.LockKey, Target.TtlSeconds,
				[Path = Target.RelativePath](bool bSuccess)
				{
					if (!bSuccess)
					{
						UE_LOG(LogGYItemEditorLock, Warning, TEXT("TTL refresh failed: %s"), *Path);
					}
				});
		}
		else
		{
			Result.bSuccess = false;
			Result.Blocked.Add({ Target.PackageName, TEXT("락 서버 통신 실패") });
			break;
		}

		if (!Target.GitPath.IsEmpty())
		{
			bool bLfsDone = false;
			PoorforceGitLfs::FLockOutcome Outcome;
			PoorforceGitLfs::TryLock(Target.GitPath,
				[&Outcome, &bLfsDone](const PoorforceGitLfs::FLockOutcome& InOutcome)
				{
					Outcome = InOutcome;
					bLfsDone = true;
				});
			PumpUntil(bLfsDone);

			if (Outcome.Result == PoorforceGitLfs::ELockResult::Success)
			{
				NewLfsPaths.Add(Target.GitPath);
				UE_LOG(LogGYItemEditorLock, Log, TEXT("LFS lock acquired: %s"), *Target.GitPath);
			}
			else if (Outcome.Result != PoorforceGitLfs::ELockResult::AlreadyOwnedByMe)
			{
				// 실패 메시지만으로는 소유자를 알 수 없는 서버가 있어 verify로 확정한다
				FString OwnerName;
				const ELfsOwnership Ownership = QueryLfsOwnership(Target.GitPath, OwnerName);

				if (Ownership == ELfsOwnership::Theirs)
				{
					Result.bSuccess = false;
					Result.Blocked.Add({ Target.PackageName,
						FString::Printf(TEXT("LFS 락 보유자: %s"), *OwnerName) });
					break;
				}

				if (Ownership == ELfsOwnership::Unknown)
				{
					Result.bSuccess = false;
					Result.Blocked.Add({ Target.PackageName, TEXT("LFS 락 취득 실패 (git/lfs 환경 확인)") });
					UE_LOG(LogGYItemEditorLock, Warning, TEXT("LFS lock failed (exit=%d): %s\n%s"),
						Outcome.ExitCode, *Target.GitPath, *Outcome.Stderr);
					break;
				}
				// Mine — 내 락 재진입, 통과
			}
		}
	}

	if (!Result.bSuccess)
	{
		// all-or-nothing: 이번에 새로 취득한 락만 되돌린다
		for (const FString& Key : NewRedisKeys)
		{
			Client->Release(Key, [](bool) {});
		}
		for (const FString& Path : NewLfsPaths)
		{
			PoorforceGitLfs::TryUnlock(Path);
		}

		for (const FBlockedPackage& Blocked : Result.Blocked)
		{
			UE_LOG(LogGYItemEditorLock, Warning, TEXT("Save blocked: %s (%s)"),
				*Blocked.PackageName, *Blocked.Reason);
		}
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE
