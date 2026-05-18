#include "AssetEditorInterceptor.h"

#include "PoorforceLog.h"
#include "LockWorkflow.h"

#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/CoreDelegates.h"

void FAssetEditorInterceptor::Register(FLockWorkflow* InWorkflow)
{
	Workflow = InWorkflow;

	TrySubscribeAll();

	if (!bSubscribedToSubsystem)
	{
		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddLambda(
			[this]()
			{
				TrySubscribeAll();
			});
	}
}

void FAssetEditorInterceptor::Unregister()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}

	if (bSubscribedToSubsystem && GEditor != nullptr)
	{
		UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (IsValid(Subsystem))
		{
			Subsystem->OnAssetEditorRequestedOpen().Remove(RequestedOpenHandle);
			Subsystem->OnAssetClosedInEditor().Remove(ClosedInEditorHandle);
			Subsystem->OnAssetEditorOpened().Remove(OpenedInEditorHandle);
		}
		bSubscribedToSubsystem = false;
	}

	if (bSubscribedToRegistry)
	{
		FAssetRegistryModule* RegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if (RegistryModule != nullptr)
		{
			IAssetRegistry& Registry = RegistryModule->Get();
			Registry.OnAssetRenamed().Remove(RenamedHandle);
			Registry.OnInMemoryAssetCreated().Remove(InMemoryCreatedHandle);
		}
		bSubscribedToRegistry = false;
	}

	Workflow = nullptr;
}

void FAssetEditorInterceptor::TrySubscribeAll()
{
	if (!bSubscribedToSubsystem)
	{
		SubscribeAssetEditorSubsystem();
	}

	if (!bSubscribedToRegistry)
	{
		SubscribeAssetRegistry();
	}
}

void FAssetEditorInterceptor::SubscribeAssetEditorSubsystem()
{
	if (GEditor == nullptr) return;

	UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!IsValid(Subsystem)) return;

	RequestedOpenHandle = Subsystem->OnAssetEditorRequestedOpen().AddRaw(this, &FAssetEditorInterceptor::OnAssetEditorRequestedOpen);
	ClosedInEditorHandle = Subsystem->OnAssetClosedInEditor().AddRaw(this, &FAssetEditorInterceptor::OnAssetClosedInEditor);

	OpenedInEditorHandle = Subsystem->OnAssetEditorOpened().AddLambda(
		[this](UObject* Asset)
		{
			if (Workflow == nullptr) return;
			Workflow->HandleAssetEditorOpened(Asset);
		});

	bSubscribedToSubsystem = true;
	UE_LOG(LogPoorforce, Log, TEXT("Subscribed to AssetEditorSubsystem events"));
}

void FAssetEditorInterceptor::SubscribeAssetRegistry()
{
	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	RenamedHandle = Registry.OnAssetRenamed().AddRaw(this, &FAssetEditorInterceptor::OnAssetRenamed);
	InMemoryCreatedHandle = Registry.OnInMemoryAssetCreated().AddRaw(this, &FAssetEditorInterceptor::OnInMemoryAssetCreated);

	bSubscribedToRegistry = true;
	UE_LOG(LogPoorforce, Log, TEXT("Subscribed to AssetRegistry events"));
}

void FAssetEditorInterceptor::OnAssetEditorRequestedOpen(UObject* Asset)
{
	if (Workflow == nullptr) return;
	Workflow->HandleAssetOpened(Asset);
}

void FAssetEditorInterceptor::OnAssetClosedInEditor(UObject* Asset, IAssetEditorInstance* /*Instance*/)
{
	if (Workflow == nullptr) return;
	Workflow->HandleAssetClosed(Asset);
}

void FAssetEditorInterceptor::OnAssetRenamed(const FAssetData& Data, const FString& OldName)
{
	if (Workflow == nullptr) return;

	FString OldPackageName = OldName;
	int32 DotIdx = INDEX_NONE;
	if (OldName.FindChar(TEXT('.'), DotIdx))
	{
		OldPackageName = OldName.Left(DotIdx);
	}

	UE_LOG(LogPoorforce, Log, TEXT("Asset renamed: '%s' -> '%s'"), *OldPackageName, *Data.PackageName.ToString());

	UObject* NewAsset = Data.GetAsset();
	Workflow->HandleAssetRenamed(OldPackageName, NewAsset);
}

void FAssetEditorInterceptor::OnInMemoryAssetCreated(UObject* Asset)
{
	if (Workflow == nullptr) return;
	if (!IsValid(Asset)) return;

	const UPackage* Package = Asset->GetPackage();
	if (!IsValid(Package)) return;

	UE_LOG(LogPoorforce, Verbose, TEXT("In-memory asset created: %s"), *Package->GetName());

	Workflow->HandleAssetOpened(Asset, /*bSkipInitialDownload=*/ true);
}
