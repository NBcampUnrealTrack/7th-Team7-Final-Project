#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

class FLockWorkflow;

class FAssetEditorInterceptor
{
public:
	void Register(FLockWorkflow* InWorkflow);
	void Unregister();

private:
	FLockWorkflow* Workflow = nullptr;

	FDelegateHandle RequestedOpenHandle;
	FDelegateHandle ClosedInEditorHandle;
	FDelegateHandle OpenedInEditorHandle;
	FDelegateHandle RenamedHandle;
	FDelegateHandle InMemoryCreatedHandle;
	FDelegateHandle PostEngineInitHandle;

	bool bSubscribedToSubsystem = false;
	bool bSubscribedToRegistry = false;

	void TrySubscribeAll();
	void SubscribeAssetEditorSubsystem();
	void SubscribeAssetRegistry();

	void OnAssetEditorRequestedOpen(UObject* Asset);
	void OnAssetClosedInEditor(UObject* Asset, class IAssetEditorInstance* Instance);
	void OnAssetRenamed(const FAssetData& Data, const FString& OldName);
	void OnInMemoryAssetCreated(UObject* Asset);
};
