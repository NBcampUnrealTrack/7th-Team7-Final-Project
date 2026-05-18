#include "PoorforceContentBrowserExtension.h"

#include "PoorforceLog.h"
#include "PoorforceConfig.h"
#include "PoorforcePathResolver.h"
#include "LockWorkflow.h"

#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "AssetRegistry/AssetData.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"

#define LOCTEXT_NAMESPACE "Poorforce"

void FPoorforceContentBrowserExtension::Register(FLockWorkflow* InWorkflow, const FPoorforceConfig* InConfig)
{
	Workflow = InWorkflow;
	Config = InConfig;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedAssets>& Extenders =
		ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	Extenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(
		this, &FPoorforceContentBrowserExtension::OnExtendAssetContextMenu));
	MenuExtenderHandle = Extenders.Last().GetHandle();

	UE_LOG(LogPoorforce, Log, TEXT("Content browser extension registered"));
}

void FPoorforceContentBrowserExtension::Unregister()
{
	if (!MenuExtenderHandle.IsValid()) return;

	if (FContentBrowserModule* ContentBrowserModule = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser")))
	{
		TArray<FContentBrowserMenuExtender_SelectedAssets>& Extenders =
			ContentBrowserModule->GetAllAssetViewContextMenuExtenders();
		Extenders.RemoveAll([this](const FContentBrowserMenuExtender_SelectedAssets& D)
		{
			return D.GetHandle() == MenuExtenderHandle;
		});
	}

	MenuExtenderHandle.Reset();
	Workflow = nullptr;
	Config = nullptr;
}

TSharedRef<FExtender> FPoorforceContentBrowserExtension::OnExtendAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	if (Config == nullptr || Workflow == nullptr) return Extender;

	// 선택된 에셋 중 LockAndSync 모드인 것 추리기
	TArray<FString> LockAndSyncPackageNames;
	for (const FAssetData& Data : SelectedAssets)
	{
		const FString PackageName = Data.PackageName.ToString();
		const FPoorforceManagedPath* Match = PoorforcePathResolver::ResolveLongestPrefix(PackageName, Config->ManagedPaths);
		if (Match != nullptr && Match->Mode == EPoorforcePathMode::LockAndSync)
		{
			LockAndSyncPackageNames.Add(PackageName);
		}
	}

	if (LockAndSyncPackageNames.Num() == 0) return Extender;

	Extender->AddMenuExtension(
		TEXT("CommonAssetActions"),
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateLambda(
			[this, LockAndSyncPackageNames](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.BeginSection("Poorforce", LOCTEXT("PoorforceSection", "Poorforce"));
				MenuBuilder.AddMenuEntry(
					LOCTEXT("Sync", "Sync (download)"),
					LOCTEXT("SyncTooltip", "최신 버전을 다운로드합니다 (메모리 캐시 갱신)"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[this, LockAndSyncPackageNames]()
						{
							if (Workflow == nullptr) return;
							for (const FString& PackageName : LockAndSyncPackageNames)
							{
								Workflow->HandleManualSync(PackageName);
							}
						})));
				MenuBuilder.EndSection();
			}));

	return Extender;
}

#undef LOCTEXT_NAMESPACE
