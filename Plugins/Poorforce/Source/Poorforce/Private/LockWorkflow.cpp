#include "LockWorkflow.h"

#include "PoorforceLog.h"
#include "PoorforceConfig.h"
#include "PoorforcePathResolver.h"
#include "PoorforceTimeFormat.h"
#include "LockServerClient.h"
#include "RcloneProcessManager.h"
#include "DetachedWatcherSpawner.h"
#include "DiscordNotifier.h"
#include "GitLfsClient.h"
#include "UI/PoorforceDialogs.h"

#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/ScopedSlowTask.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "UObject/Object.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"

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

	void ReopenAssetEditor(TWeakObjectPtr<UObject> WeakAsset)
	{
		if (GEditor == nullptr) return;
		UObject* Asset = WeakAsset.Get();
		if (!IsValid(Asset)) return;
		UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (!IsValid(Subsystem)) return;
		Subsystem->OpenEditorForAsset(Asset);
	}
}

FLockWorkflow::FLockWorkflow(
	const FPoorforceConfig& InConfig,
	TSharedPtr<FLockServerClient> InClient,
	TSharedPtr<FRcloneProcessManager> InRclone,
	TSharedPtr<FDetachedWatcherSpawner> InWatcher,
	FString InUserId)
	: Config(InConfig)
	, Client(MoveTemp(InClient))
	, Rclone(MoveTemp(InRclone))
	, Watcher(MoveTemp(InWatcher))
	, UserId(MoveTemp(InUserId))
{
	PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddLambda(
		[this](const FString&, UPackage* Package, FObjectPostSaveContext)
		{
			if (!IsValid(Package)) return;
			SavedPackageNamesThisSession.Add(Package->GetName());
		});
}

FLockWorkflow::~FLockWorkflow()
{
	if (PackageSavedHandle.IsValid())
	{
		UPackage::PackageSavedWithContextEvent.Remove(PackageSavedHandle);
		PackageSavedHandle.Reset();
	}
}

int32 FLockWorkflow::GetTtlForMode(EPoorforcePathMode Mode) const
{
	return Mode == EPoorforcePathMode::LockOnly
		? Config.LockOnlyTtlSeconds
		: Config.LockAndSyncTtlSeconds;
}

TArray<FString> FLockWorkflow::GetOwnedLockKeys() const
{
	TArray<FString> Out;
	Out.Reserve(OwnedLockKeys.Num());
	for (const FString& Key : OwnedLockKeys)
	{
		Out.Add(Key);
	}
	Out.Sort();
	return Out;
}

void FLockWorkflow::ManualReleaseLock(const FString& LockKey)
{
	if (!Client.IsValid())
	{
		UE_LOG(LogPoorforce, Warning, TEXT("ManualReleaseLock: client not initialised"));
		return;
	}

	if (!OwnedLockKeys.Contains(LockKey))
	{
		UE_LOG(LogPoorforce, Warning, TEXT("ManualReleaseLock: '%s' is not owned by this session"), *LockKey);
		return;
	}

	// 참고: 여기서는 LFS unlock 안 함 (LockKey만으로 디스크 파일 경로 정확히 못 구함).
	// LFS unlock 은 닫기 시(변경 없음 분기) 또는 CI의 release-lfs-locks.sh 에서 처리.
	Client->Release(LockKey,
		[this, LockKey](bool bReleased)
		{
			OwnedLockKeys.Remove(LockKey);
			if (Watcher.IsValid()) Watcher->SignalWatcherExit(LockKey);

			if (bReleased)
			{
				UE_LOG(LogPoorforce, Log, TEXT("Manual release succeeded: %s"), *LockKey);
			}
			else
			{
				UE_LOG(LogPoorforce, Warning, TEXT("Manual release failed (removed from owned set anyway): %s"), *LockKey);
			}
		});
}

bool FLockWorkflow::Resolve(UObject* Asset, FResolvedAsset& Out) const
{
	if (!IsValid(Asset)) return false;

	UPackage* Package = Asset->GetPackage();
	if (!IsValid(Package)) return false;

	Out.PackageName = Package->GetName();

	Out.Match = PoorforcePathResolver::ResolveLongestPrefix(Out.PackageName, Config.ManagedPaths);
	if (Out.Match == nullptr) return false;

	Out.RelativePath = PoorforcePathResolver::MakeRelativePath(Out.PackageName, *Out.Match);
	Out.LockKey      = PoorforcePathResolver::MakeLockKey(Config.LockKeyNamespace, Out.RelativePath);

	if (Out.Match->Mode == EPoorforcePathMode::LockAndSync)
	{
		const FString Extension = PoorforcePathResolver::GetPackageExtensionFor(Asset);
		PoorforcePathResolver::MakeLocalFilePath(Out.PackageName, Asset, Out.LocalFilePath);
		Out.RemoteFilePath = PoorforcePathResolver::MakeRemoteFilePath(Out.Match->RcloneRemote, Out.RelativePath, Extension);

		const TArray<PoorforcePathResolver::FSidecarPair> Sidecars =
			PoorforcePathResolver::MakeSidecarPaths(Out.PackageName, Asset, Out.Match->RcloneRemote, Out.RelativePath);
		for (const PoorforcePathResolver::FSidecarPair& Pair : Sidecars)
		{
			Out.SidecarPaths.Emplace(Pair.LocalPath, Pair.RemotePath);
		}
	}
	else if (Out.Match->Mode == EPoorforcePathMode::LockOnly)
	{
		// LockOnly에서 git lfs lock 인자로 쓸 프로젝트 루트 기준 상대경로 계산
		FString AbsoluteFile;
		if (PoorforcePathResolver::MakeLocalFilePath(Out.PackageName, Asset, AbsoluteFile))
		{
			const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
			FString GitPath = FPaths::ConvertRelativePathToFull(AbsoluteFile);
			if (GitPath.StartsWith(ProjectDir, ESearchCase::IgnoreCase))
			{
				GitPath = GitPath.RightChop(ProjectDir.Len());
			}
			GitPath.ReplaceInline(TEXT("\\"), TEXT("/"));
			while (GitPath.StartsWith(TEXT("/"))) GitPath.RightChopInline(1);
			Out.GitRelativeFilePath = GitPath;
		}
	}

	return true;
}

void FLockWorkflow::HandleAssetOpened(UObject* Asset, bool bSkipInitialDownload)
{
	if (!Client.IsValid()) return;

	FResolvedAsset Resolved;
	if (!Resolve(Asset, Resolved)) return;

	// 에디터 toolkit 초기화 동안 일부 에디터(예: AnimSequence)는 내부적으로
	// RemoveEditingObject → AddEditingObject 를 호출해서 close/open 이벤트를 발화함.
	// OnAssetEditorOpened 가 발화할 때까지 close는 무시.
	OpeningAssetPackageNames.Add(Resolved.PackageName);

	if (InFlightSyncs.Contains(Resolved.LockKey))
	{
		UE_LOG(LogPoorforce, Verbose, TEXT("Skip open handling — operation in flight: %s"), *Resolved.RelativePath);
		return;
	}

	if (bSkipInitialDownload)
	{
		AcquireAsyncNoDownload(Asset, Resolved);
		return;
	}

	SyncPrefetch(Asset, Resolved);
}

void FLockWorkflow::HandleAssetEditorOpened(UObject* Asset)
{
	if (!IsValid(Asset)) return;
	const UPackage* Package = Asset->GetPackage();
	if (!IsValid(Package)) return;
	OpeningAssetPackageNames.Remove(Package->GetName());
}

void FLockWorkflow::SyncPrefetch(UObject* Asset, const FResolvedAsset& Resolved)
{
	const FString MyUserId = UserId;
	TWeakObjectPtr<UObject> WeakAsset(Asset);
	const int32 Ttl = GetTtlForMode(Resolved.Match->Mode);

	const bool bIsLockAndSync = Resolved.Match->Mode == EPoorforcePathMode::LockAndSync;
	const float TotalWork = bIsLockAndSync ? 2.f : 1.f;

	FScopedSlowTask Task(TotalWork, FText::FromString(TEXT("Poorforce — 락 획득 중...")));
	Task.MakeDialogDelayed(0.5f);

	// Phase 1: TryAcquire (sync wait)
	bool bAcquireDone = false;
	PoorforceLock::EAcquireResult AcquireResult = PoorforceLock::EAcquireResult::NetworkError;

	Client->TryAcquire(Resolved.LockKey, MyUserId, Ttl,
		[&bAcquireDone, &AcquireResult](PoorforceLock::EAcquireResult R)
		{
			AcquireResult = R;
			bAcquireDone = true;
		});

	PumpUntil(bAcquireDone);
	Task.EnterProgressFrame(1.f);

	if (AcquireResult == PoorforceLock::EAcquireResult::Acquired)
	{
		OwnedLockKeys.Add(Resolved.LockKey);
		UE_LOG(LogPoorforce, Log, TEXT("[Case 1] Lock acquired: %s"), *Resolved.RelativePath);

		MaybeAcquireLfsLock(Resolved);

		if (bIsLockAndSync)
		{
			if (Watcher.IsValid())
			{
				Watcher->SpawnWatcher(Resolved.LockKey, Resolved.LocalFilePath, Resolved.RemoteFilePath);
			}

			Task.EnterProgressFrame(0.f, FText::FromString(FString::Printf(
				TEXT("다운로드 중: %s"), *Resolved.RelativePath)));

			const bool bDownloaded = DownloadSync(Resolved.LocalFilePath, Resolved.RemoteFilePath);

			if (bDownloaded)
			{
				for (const TPair<FString, FString>& Pair : Resolved.SidecarPaths)
				{
					DownloadSync(Pair.Key, Pair.Value);
				}
			}
			else
			{
				UE_LOG(LogPoorforce, Warning,
					TEXT("Download failed for %s. Releasing lock so others aren't blocked."),
					*Resolved.RelativePath);

				NotifyUserWarning(FString::Printf(TEXT("Download failed: %s"), *Resolved.RelativePath));

				Client->Release(Resolved.LockKey,
					[this, Key = Resolved.LockKey](bool)
					{
						OwnedLockKeys.Remove(Key);
						if (Watcher.IsValid()) Watcher->SignalWatcherExit(Key);
					});
			}

			Task.EnterProgressFrame(1.f);
		}

		return;
	}

	if (AcquireResult == PoorforceLock::EAcquireResult::NetworkError)
	{
		UE_LOG(LogPoorforce, Warning, TEXT("Lock acquire network error: %s"), *Resolved.RelativePath);
		return;
	}

	// AlreadyHeld: sync GET to check ownership
	bool bGetDone = false;
	TOptional<PoorforceLock::FLockEntry> Entry;

	Client->Get(Resolved.LockKey,
		[&bGetDone, &Entry](bool bExists, const TOptional<PoorforceLock::FLockEntry>& Ent)
		{
			if (bExists) Entry = Ent;
			bGetDone = true;
		});

	PumpUntil(bGetDone);

	if (!Entry.IsSet())
	{
		UE_LOG(LogPoorforce, Warning, TEXT("Lock vanished between SET and GET: %s"), *Resolved.RelativePath);
		return;
	}

	if (Entry->OwnerId.Equals(MyUserId, ESearchCase::CaseSensitive))
	{
		// Case 2: re-entry
		OwnedLockKeys.Add(Resolved.LockKey);
		UE_LOG(LogPoorforce, Log, TEXT("[Case 2] Lock re-entry, refreshing TTL: %s"), *Resolved.RelativePath);

		if (bIsLockAndSync && Watcher.IsValid() && !Watcher->IsWatcherActive(Resolved.LockKey))
		{
			Watcher->SpawnWatcher(Resolved.LockKey, Resolved.LocalFilePath, Resolved.RemoteFilePath);
		}

		Client->Refresh(Resolved.LockKey, Ttl,
			[Path = Resolved.RelativePath](bool bSuccess)
			{
				if (!bSuccess)
				{
					UE_LOG(LogPoorforce, Warning, TEXT("Lock TTL refresh failed: %s"), *Path);
				}
			});
		return;
	}

	// Case 3: blocked. Editor is about to open (we haven't returned yet).
	// Defer to next tick so the dialog and CloseAllEditorsForAsset run AFTER
	// the editor instance is actually created.
	const PoorforceLock::FLockEntry EntryCopy = *Entry;
	const FResolvedAsset ResolvedCopy = Resolved;

	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda(
			[this, ResolvedCopy, EntryCopy, WeakAsset](float) -> bool
			{
				HandleBlockedByOther(ResolvedCopy, EntryCopy, WeakAsset);
				return false;
			}),
		0.0f);
}

void FLockWorkflow::AcquireAsyncNoDownload(UObject* Asset, const FResolvedAsset& Resolved)
{
	const FString MyUserId = UserId;
	TWeakObjectPtr<UObject> WeakAsset(Asset);

	Client->TryAcquire(Resolved.LockKey, MyUserId, GetTtlForMode(Resolved.Match->Mode),
		[this, Resolved, MyUserId, WeakAsset](PoorforceLock::EAcquireResult Result)
		{
			switch (Result)
			{
			case PoorforceLock::EAcquireResult::Acquired:
				OwnedLockKeys.Add(Resolved.LockKey);
				UE_LOG(LogPoorforce, Log, TEXT("[Case 1] Lock acquired: %s"), *Resolved.RelativePath);

				MaybeAcquireLfsLock(Resolved);

				if (Resolved.Match->Mode == EPoorforcePathMode::LockAndSync && Watcher.IsValid())
				{
					Watcher->SpawnWatcher(Resolved.LockKey, Resolved.LocalFilePath, Resolved.RemoteFilePath);
				}
				return;

			case PoorforceLock::EAcquireResult::NetworkError:
				UE_LOG(LogPoorforce, Warning, TEXT("Lock acquire network error: %s"), *Resolved.RelativePath);
				return;

			case PoorforceLock::EAcquireResult::AlreadyHeld:
				break;
			}

			Client->Get(Resolved.LockKey,
				[this, Resolved, MyUserId, WeakAsset](bool bExists, const TOptional<PoorforceLock::FLockEntry>& Entry)
				{
					if (!bExists || !Entry.IsSet())
					{
						UE_LOG(LogPoorforce, Warning, TEXT("Lock vanished between SET and GET: %s"), *Resolved.RelativePath);
						return;
					}

					if (Entry->OwnerId.Equals(MyUserId, ESearchCase::CaseSensitive))
					{
						OwnedLockKeys.Add(Resolved.LockKey);
						UE_LOG(LogPoorforce, Log, TEXT("[Case 2] Lock re-entry, refreshing TTL: %s"), *Resolved.RelativePath);

						if (Resolved.Match->Mode == EPoorforcePathMode::LockAndSync && Watcher.IsValid()
							&& !Watcher->IsWatcherActive(Resolved.LockKey))
						{
							Watcher->SpawnWatcher(Resolved.LockKey, Resolved.LocalFilePath, Resolved.RemoteFilePath);
						}

						Client->Refresh(Resolved.LockKey, GetTtlForMode(Resolved.Match->Mode),
							[Path = Resolved.RelativePath](bool ok)
							{
								if (!ok) UE_LOG(LogPoorforce, Warning, TEXT("Lock TTL refresh failed: %s"), *Path);
							});
						return;
					}

					HandleBlockedByOther(Resolved, *Entry, WeakAsset);
				});
		});
}

bool FLockWorkflow::DownloadSync(const FString& LocalPath, const FString& RemotePath)
{
	if (!Rclone.IsValid()) return false;
	if (LocalPath.IsEmpty() || RemotePath.IsEmpty()) return false;

	bool bDone = false;
	bool bSuccess = false;

	Rclone->StartCopyTo(
		FRcloneProcessManager::EDirection::Download,
		LocalPath,
		RemotePath,
		[&bDone, &bSuccess](bool ok, int32, const FString&)
		{
			bSuccess = ok;
			bDone = true;
		});

	PumpUntil(bDone);

	if (bSuccess)
	{
		UE_LOG(LogPoorforce, Log, TEXT("Download complete: %s"), *RemotePath);
	}

	return bSuccess;
}

void FLockWorkflow::HandleAssetClosed(UObject* Asset)
{
	if (!Client.IsValid()) return;

	FResolvedAsset Resolved;
	if (!Resolve(Asset, Resolved)) return;

	if (OpeningAssetPackageNames.Contains(Resolved.PackageName))
	{
		UE_LOG(LogPoorforce, Verbose, TEXT("Close suppressed (toolkit still initialising): %s"), *Resolved.RelativePath);
		return;
	}

	if (InFlightSyncs.Contains(Resolved.LockKey))
	{
		UE_LOG(LogPoorforce, Verbose, TEXT("Ignore close — operation in flight: %s"), *Resolved.RelativePath);
		return;
	}

	if (!OwnedLockKeys.Contains(Resolved.LockKey))
	{
		return;
	}

	if (Resolved.Match->Mode == EPoorforcePathMode::LockAndSync)
	{
		StartUploadAndRelease(Resolved);
		return;
	}

	// LockOnly: 세션 중 저장이 한 번도 없었으면 (=변경 없음) 락 즉시 해제.
	// 저장된 적이 있으면 유지 (PR 머지 전까지) → 콘솔 커맨드 또는 CI에서 해제.
	if (!SavedPackageNamesThisSession.Contains(Resolved.PackageName))
	{
		UE_LOG(LogPoorforce, Log, TEXT("LockOnly close — no changes, releasing lock: %s"), *Resolved.RelativePath);

		MaybeReleaseLfsLock(Resolved);

		Client->Release(Resolved.LockKey,
			[this, Key = Resolved.LockKey, Path = Resolved.RelativePath](bool bReleased)
			{
				OwnedLockKeys.Remove(Key);

				if (bReleased)
				{
					UE_LOG(LogPoorforce, Log, TEXT("Lock released (no changes): %s"), *Path);
				}
				else
				{
					UE_LOG(LogPoorforce, Warning, TEXT("Lock release failed (no changes): %s"), *Path);
				}
			});
		return;
	}

	UE_LOG(LogPoorforce, Log, TEXT("LockOnly close — keeping lock (changes saved this session): %s"), *Resolved.RelativePath);
}

void FLockWorkflow::HandleAssetRenamed(const FString& OldPackageName, UObject* NewAsset)
{
	if (!Client.IsValid()) return;

	const FPoorforceManagedPath* OldMatch = PoorforcePathResolver::ResolveLongestPrefix(OldPackageName, Config.ManagedPaths);
	if (OldMatch != nullptr)
	{
		const FString OldRelative = PoorforcePathResolver::MakeRelativePath(OldPackageName, *OldMatch);
		const FString OldKey      = PoorforcePathResolver::MakeLockKey(Config.LockKeyNamespace, OldRelative);

		if (OwnedLockKeys.Contains(OldKey))
		{
			UE_LOG(LogPoorforce, Log, TEXT("Releasing old lock after rename: %s"), *OldRelative);

			Client->Release(OldKey,
				[this, OldKey, OldRelative](bool bReleased)
				{
					OwnedLockKeys.Remove(OldKey);
					if (Watcher.IsValid()) Watcher->SignalWatcherExit(OldKey);

					if (bReleased)
					{
						UE_LOG(LogPoorforce, Log, TEXT("Old lock released after rename: %s"), *OldRelative);
					}
					else
					{
						UE_LOG(LogPoorforce, Warning, TEXT("Old lock release after rename failed: %s"), *OldRelative);
					}
				});
		}
	}

	if (IsValid(NewAsset))
	{
		HandleAssetOpened(NewAsset, /*bSkipInitialDownload=*/ true);
	}
}

void FLockWorkflow::HandleBlockedByOther(
	const FResolvedAsset& Resolved,
	const PoorforceLock::FLockEntry& Entry,
	TWeakObjectPtr<UObject> WeakAsset)
{
	const FString ElapsedText = PoorforceTimeFormat::FormatElapsedSinceTimestampString(Entry.Timestamp);

	UE_LOG(LogPoorforce, Warning,
		TEXT("[Case 3] BLOCKED: '%s' is held by '%s' (%s)"),
		*Resolved.RelativePath, *Entry.OwnerId, *ElapsedText);

	CloseAssetEditor(WeakAsset);

	const EBlockedDialogResult Choice = PoorforceDialogs::ShowBlockedDialog(
		Resolved.RelativePath, Entry.OwnerId, ElapsedText);

	if (Choice != EBlockedDialogResult::ForceUnlockRequested)
	{
		return;
	}

	const FForceUnlockDialogResult ForceResult = PoorforceDialogs::ShowForceUnlockDialog(
		Resolved.RelativePath, Entry.OwnerId);

	if (!ForceResult.bConfirmed)
	{
		return;
	}

	UE_LOG(LogPoorforce, Warning,
		TEXT("Force unlock: '%s' (was held by '%s') by '%s'. Reason: %s"),
		*Resolved.RelativePath, *Entry.OwnerId, *UserId,
		ForceResult.Reason.IsEmpty() ? TEXT("(없음)") : *ForceResult.Reason);

	const FString OriginalOwner = Entry.OwnerId;
	const FString ReasonText = ForceResult.Reason;
	const FString WebhookUrl = Config.DiscordWebhookUrl;
	const FString Actor = UserId;

	Client->Release(Resolved.LockKey,
		[this, Resolved, WeakAsset, OriginalOwner, ReasonText, WebhookUrl, Actor](bool bReleased)
		{
			if (!bReleased)
			{
				UE_LOG(LogPoorforce, Warning, TEXT("Force unlock DEL failed: %s"), *Resolved.RelativePath);
				NotifyUserWarning(FString::Printf(TEXT("강제 해제 실패: %s"), *Resolved.RelativePath));
				return;
			}

			UE_LOG(LogPoorforce, Log, TEXT("Force unlock succeeded, reopening: %s"), *Resolved.RelativePath);

			PoorforceDiscord::SendForceUnlockNotice(
				WebhookUrl, Resolved.RelativePath, OriginalOwner, Actor, ReasonText);

			ReopenAssetEditor(WeakAsset);
		});
}

void FLockWorkflow::StartUploadAndRelease(const FResolvedAsset& Resolved)
{
	if (!Rclone.IsValid())
	{
		UE_LOG(LogPoorforce, Warning, TEXT("LockAndSync close but rclone manager missing — releasing lock without upload: %s"), *Resolved.RelativePath);

		Client->Release(Resolved.LockKey,
			[this, Key = Resolved.LockKey](bool)
			{
				OwnedLockKeys.Remove(Key);
				if (Watcher.IsValid()) Watcher->SignalWatcherExit(Key);
			});
		return;
	}

	if (Resolved.LocalFilePath.IsEmpty() || Resolved.RemoteFilePath.IsEmpty())
	{
		UE_LOG(LogPoorforce, Warning, TEXT("LockAndSync close path resolution failed — releasing lock without upload: %s"), *Resolved.RelativePath);

		Client->Release(Resolved.LockKey,
			[this, Key = Resolved.LockKey](bool)
			{
				OwnedLockKeys.Remove(Key);
				if (Watcher.IsValid()) Watcher->SignalWatcherExit(Key);
			});
		return;
	}

	InFlightSyncs.Add(Resolved.LockKey);

	NotifyUser(FString::Printf(TEXT("Uploading %s..."), *Resolved.RelativePath));

	Rclone->StartCopyTo(
		FRcloneProcessManager::EDirection::Upload,
		Resolved.LocalFilePath,
		Resolved.RemoteFilePath,
		[this, Resolved](bool bSuccess, int32 ExitCode, const FString& Output)
		{
			InFlightSyncs.Remove(Resolved.LockKey);

			if (!bSuccess)
			{
				UE_LOG(LogPoorforce, Warning,
					TEXT("Upload failed for %s (exit=%d). Asking user."),
					*Resolved.RelativePath, ExitCode);

				const EUploadRetryChoice Choice = PoorforceDialogs::ShowUploadRetryDialog(
					Resolved.RelativePath, ExitCode, Output);

				switch (Choice)
				{
				case EUploadRetryChoice::Retry:
					UE_LOG(LogPoorforce, Log, TEXT("User chose retry: %s"), *Resolved.RelativePath);
					StartUploadAndRelease(Resolved);
					return;

				case EUploadRetryChoice::ReleaseAnyway:
					UE_LOG(LogPoorforce, Warning, TEXT("User chose release-without-upload: %s"), *Resolved.RelativePath);
					Client->Release(Resolved.LockKey,
						[this, Key = Resolved.LockKey](bool)
						{
							OwnedLockKeys.Remove(Key);
							if (Watcher.IsValid()) Watcher->SignalWatcherExit(Key);
						});
					return;

				case EUploadRetryChoice::Dismiss:
					UE_LOG(LogPoorforce, Warning, TEXT("User dismissed; lock kept (TTL will release): %s"), *Resolved.RelativePath);
					return;
				}

				return;
			}

			UE_LOG(LogPoorforce, Log, TEXT("Upload complete: %s"), *Resolved.RelativePath);

			CopyNextSidecar(Resolved, static_cast<int32>(FRcloneProcessManager::EDirection::Upload), 0,
				[this, Resolved]()
				{
					Client->Release(Resolved.LockKey,
						[this, Key = Resolved.LockKey, Path = Resolved.RelativePath](bool bReleased)
						{
							OwnedLockKeys.Remove(Key);
							if (Watcher.IsValid()) Watcher->SignalWatcherExit(Key);

							if (bReleased)
							{
								UE_LOG(LogPoorforce, Log, TEXT("Lock released after upload: %s"), *Path);
							}
							else
							{
								UE_LOG(LogPoorforce, Warning, TEXT("Lock release after upload failed (removed from owned set anyway): %s"), *Path);
							}
						});
				});
		});
}

void FLockWorkflow::CopyNextSidecar(
	FResolvedAsset Resolved,
	int32 Direction,
	int32 Index,
	TFunction<void()> OnAllDone)
{
	if (Index >= Resolved.SidecarPaths.Num() || !Rclone.IsValid())
	{
		OnAllDone();
		return;
	}

	const TPair<FString, FString>& Pair = Resolved.SidecarPaths[Index];

	Rclone->StartCopyTo(
		static_cast<FRcloneProcessManager::EDirection>(Direction),
		Pair.Key,
		Pair.Value,
		[this, Resolved = MoveTemp(Resolved), Direction, Index, OnAllDone = MoveTemp(OnAllDone)](bool bSuccess, int32 ExitCode, const FString& Output) mutable
		{
			if (!bSuccess)
			{
				UE_LOG(LogPoorforce, Warning,
					TEXT("Sidecar copy failed (best-effort, continuing): exit=%d for %s"),
					ExitCode, *Resolved.SidecarPaths[Index].Key);
			}

			CopyNextSidecar(MoveTemp(Resolved), Direction, Index + 1, MoveTemp(OnAllDone));
		});
}

void FLockWorkflow::NotifyUser(const FString& Message) const
{
	FNotificationInfo Info(FText::FromString(Message));
	Info.bFireAndForget = true;
	Info.ExpireDuration = 4.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

void FLockWorkflow::NotifyUserWarning(const FString& Message) const
{
	FNotificationInfo Info(FText::FromString(Message));
	Info.bFireAndForget = true;
	Info.ExpireDuration = 6.0f;
	TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
	if (Item.IsValid())
	{
		Item->SetCompletionState(SNotificationItem::CS_Fail);
	}
}

void FLockWorkflow::MaybeAcquireLfsLock(const FResolvedAsset& Resolved)
{
	if (Resolved.Match == nullptr || Resolved.Match->Mode != EPoorforcePathMode::LockOnly) return;
	if (Resolved.GitRelativeFilePath.IsEmpty()) return;

	const FString GitPath      = Resolved.GitRelativeFilePath;
	const FString RelativePath = Resolved.RelativePath;

	PoorforceGitLfs::TryLock(GitPath,
		[this, GitPath, RelativePath](const PoorforceGitLfs::FLockOutcome& Outcome)
		{
			switch (Outcome.Result)
			{
			case PoorforceGitLfs::ELockResult::Success:
				UE_LOG(LogPoorforce, Log, TEXT("LFS lock acquired: %s"), *GitPath);
				return;

			case PoorforceGitLfs::ELockResult::AlreadyOwnedByMe:
				UE_LOG(LogPoorforce, Verbose, TEXT("LFS lock already mine: %s"), *GitPath);
				return;

			case PoorforceGitLfs::ELockResult::OwnedByOther:
				UE_LOG(LogPoorforce, Warning, TEXT("LFS lock owned by other (exit=%d): %s\n%s"),
					Outcome.ExitCode, *GitPath, *Outcome.Stderr);
				NotifyUserWarning(FString::Printf(TEXT("LFS 락이 다른 사람에게 있음: %s"), *RelativePath));
				return;

			case PoorforceGitLfs::ELockResult::SetupError:
				UE_LOG(LogPoorforce, Warning, TEXT("LFS lock setup error (exit=%d): %s\n%s"),
					Outcome.ExitCode, *GitPath, *Outcome.Stderr);
				NotifyUserWarning(FString::Printf(TEXT("LFS 셋업 문제로 락 실패: %s"), *RelativePath));
				return;

			case PoorforceGitLfs::ELockResult::Other:
			default:
				UE_LOG(LogPoorforce, Warning, TEXT("LFS lock failed (exit=%d): %s\n%s"),
					Outcome.ExitCode, *GitPath, *Outcome.Stderr);
				NotifyUserWarning(FString::Printf(TEXT("LFS 락 실패: %s"), *RelativePath));
				return;
			}
		});
}

void FLockWorkflow::MaybeReleaseLfsLock(const FResolvedAsset& Resolved)
{
	if (Resolved.Match == nullptr || Resolved.Match->Mode != EPoorforcePathMode::LockOnly) return;
	if (Resolved.GitRelativeFilePath.IsEmpty()) return;

	PoorforceGitLfs::TryUnlock(Resolved.GitRelativeFilePath);
}

void FLockWorkflow::CloseAssetEditor(TWeakObjectPtr<UObject> WeakAsset)
{
	if (GEditor == nullptr) return;

	UObject* Asset = WeakAsset.Get();
	if (!IsValid(Asset)) return;

	UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!IsValid(Subsystem)) return;

	Subsystem->CloseAllEditorsForAsset(Asset);
}
