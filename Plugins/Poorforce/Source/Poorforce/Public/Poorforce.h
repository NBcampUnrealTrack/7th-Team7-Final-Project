#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "PoorforceConfig.h"

class FLockServerClient;
class FRcloneProcessManager;
class FDetachedWatcherSpawner;
class FLockWorkflow;
class FAssetEditorInterceptor;

class FPoorforceModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FPoorforceModule& Get();

	bool IsEnabled() const { return bEnabled; }
	const FPoorforceConfig& GetConfig() const { return Config; }
	TSharedPtr<FLockServerClient> GetLockClient() const { return LockClient; }
	TSharedPtr<FRcloneProcessManager> GetRcloneManager() const { return Rclone; }

private:
	bool bEnabled = false;
	FPoorforceConfig Config;
	TSharedPtr<FLockServerClient> LockClient;
	TSharedPtr<FRcloneProcessManager> Rclone;
	TSharedPtr<FDetachedWatcherSpawner> Watcher;
	TUniquePtr<FLockWorkflow> Workflow;
	TUniquePtr<FAssetEditorInterceptor> Interceptor;

	FDelegateHandle PreExitHandle;

	TArray<class IConsoleObject*> RegisteredConsoleCommands;
};
