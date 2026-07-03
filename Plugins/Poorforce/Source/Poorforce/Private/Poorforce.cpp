#include "Poorforce.h"

#include "PoorforceLog.h"
#include "LockServerClient.h"
#include "RcloneProcessManager.h"
#include "DetachedWatcherSpawner.h"
#include "LockWorkflow.h"
#include "AssetEditorInterceptor.h"
#include "PoorforceContentBrowserExtension.h"
#include "UserIdProvider.h"
#include "UI/PoorforceDialogs.h"

#include "Editor.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogPoorforce);

#define LOCTEXT_NAMESPACE "FPoorforceModule"

void FPoorforceModule::StartupModule()
{
	UE_LOG(LogPoorforce, Log, TEXT("Poorforce starting up..."));

	if (!PoorforceConfigLoader::LoadFromProjectRoot(Config))
	{
		UE_LOG(LogPoorforce, Warning, TEXT("Poorforce disabled — config not loaded. Expected at: %s"),
			*PoorforceConfigLoader::GetExpectedConfigPath());
		bEnabled = false;
		return;
	}

	const FString& UserId = PoorforceUserId::Get();

	LockClient = MakeShared<FLockServerClient>(Config.UpstashUrl, Config.UpstashToken);
	Rclone = MakeShared<FRcloneProcessManager>(Config.RcloneExecutable);
	Watcher = MakeShared<FDetachedWatcherSpawner>(Config.RcloneExecutable, Config.UpstashUrl, Config.UpstashToken, UserId);
	Watcher->CleanupStaleArtifacts();
	Workflow = MakeUnique<FLockWorkflow>(Config, LockClient, Rclone, Watcher, UserId);

	Interceptor = MakeUnique<FAssetEditorInterceptor>();
	Interceptor->Register(Workflow.Get());

	ContentBrowserExtension = MakeUnique<FPoorforceContentBrowserExtension>();
	ContentBrowserExtension->Register(Workflow.Get(), &Config);

	PreExitHandle = FEditorDelegates::OnEditorPreExit.AddLambda(
		[this]()
		{
			if (Rclone.IsValid() && Rclone->HasActive())
			{
				PoorforceDialogs::WaitForUploadsModal(Rclone);
			}
		});

	RegisteredConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Poorforce.ListLocks"),
		TEXT("List locks you currently hold, read live from Redis (not just this editor session)."),
		FConsoleCommandDelegate::CreateLambda(
			[this]()
			{
				if (!LockClient.IsValid())
				{
					UE_LOG(LogPoorforce, Display, TEXT("Poorforce not initialised"));
					return;
				}

				// 세션 메모리가 아닌 Redis 에서 직접 읽음 → 에디터 재시작 후에도 정확.
				const FString MatchPattern = Config.LockKeyNamespace.IsEmpty()
					? TEXT("asset:*")
					: FString::Printf(TEXT("%s:asset:*"), *Config.LockKeyNamespace);
				const FString Owner = PoorforceUserId::Get();

				UE_LOG(LogPoorforce, Display, TEXT("Querying Redis for locks owned by '%s'..."), *Owner);

				LockClient->ListOwnedKeys(MatchPattern, Owner,
					[Owner](bool bSuccess, const TArray<FString>& Keys)
					{
						if (!bSuccess)
						{
							UE_LOG(LogPoorforce, Warning, TEXT("ListLocks: lock server query failed"));
							return;
						}

						if (Keys.Num() == 0)
						{
							UE_LOG(LogPoorforce, Display, TEXT("No owned locks (owner=%s)"), *Owner);
							return;
						}

						UE_LOG(LogPoorforce, Display, TEXT("Owned locks (%d):"), Keys.Num());
						for (const FString& Key : Keys)
						{
							UE_LOG(LogPoorforce, Display, TEXT("  %s"), *Key);
						}
					});
			}),
		ECVF_Default));

	RegisteredConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Poorforce.ReleaseLock"),
		TEXT("Manually release a lock by key. Usage: Poorforce.ReleaseLock <key>"),
		FConsoleCommandWithArgsDelegate::CreateLambda(
			[this](const TArray<FString>& Args)
			{
				if (!Workflow.IsValid())
				{
					UE_LOG(LogPoorforce, Display, TEXT("Poorforce not initialised"));
					return;
				}

				if (Args.Num() < 1)
				{
					UE_LOG(LogPoorforce, Display, TEXT("Usage: Poorforce.ReleaseLock <key>"));
					return;
				}

				Workflow->ManualReleaseLock(Args[0]);
			}),
		ECVF_Default));

	bEnabled = true;

	UE_LOG(LogPoorforce, Log, TEXT("Poorforce ready. UserId='%s', ManagedPaths=%d, Rclone='%s'"),
		*UserId, Config.ManagedPaths.Num(), *Config.RcloneExecutable);
}

void FPoorforceModule::ShutdownModule()
{
	for (IConsoleObject* Cmd : RegisteredConsoleCommands)
	{
		if (Cmd != nullptr)
		{
			IConsoleManager::Get().UnregisterConsoleObject(Cmd);
		}
	}
	RegisteredConsoleCommands.Empty();

	if (PreExitHandle.IsValid())
	{
		FEditorDelegates::OnEditorPreExit.Remove(PreExitHandle);
		PreExitHandle.Reset();
	}

	if (ContentBrowserExtension.IsValid())
	{
		ContentBrowserExtension->Unregister();
		ContentBrowserExtension.Reset();
	}

	if (Interceptor.IsValid())
	{
		Interceptor->Unregister();
		Interceptor.Reset();
	}

	Workflow.Reset();

	if (Rclone.IsValid())
	{
		Rclone->CancelAll();
		Rclone.Reset();
	}

	Watcher.Reset();
	LockClient.Reset();
	bEnabled = false;

	UE_LOG(LogPoorforce, Log, TEXT("Poorforce shut down."));
}

FPoorforceModule& FPoorforceModule::Get()
{
	return FModuleManager::LoadModuleChecked<FPoorforceModule>(TEXT("Poorforce"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPoorforceModule, Poorforce)
