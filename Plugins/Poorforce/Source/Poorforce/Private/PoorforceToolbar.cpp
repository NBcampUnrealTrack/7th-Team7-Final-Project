#include "PoorforceToolbar.h"

#include "UI/SPoorforceLockWindow.h"

#include "ToolMenus.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "PoorforceToolbar"

namespace
{
	const FName PoorforceStatusTabId(TEXT("PoorforceLockStatus"));
	const FName PoorforceMenuOwner(TEXT("Poorforce"));
}

void FPoorforceToolbar::Register()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateStatic(&FPoorforceToolbar::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		PoorforceStatusTabId,
		FOnSpawnTab::CreateStatic(&FPoorforceToolbar::SpawnStatusTab))
		.SetDisplayName(LOCTEXT("StatusTabTitle", "Poorforce Locks"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FPoorforceToolbar::Unregister()
{
	// 엔진 종료 중엔 UToolMenus::Get()이 nullptr — 가드 필수
	if (!UObjectInitialized() || IsEngineExitRequested()) return;

	if (UToolMenus* ToolMenus = UToolMenus::Get())
	{
		ToolMenus->UnregisterOwnerByName(PoorforceMenuOwner);
	}

	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PoorforceStatusTabId);
	}
}

void FPoorforceToolbar::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(PoorforceMenuOwner);

	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
	FToolMenuSection& Section = MainMenu->AddSection("Poorforce");

	Section.AddSubMenu(
		"PoorforceMenu",
		LOCTEXT("MenuLabel", "Poorforce"),
		LOCTEXT("MenuTooltip", "Poorforce actions"),
		FNewMenuDelegate::CreateStatic(&FPoorforceToolbar::BuildDropdownMenu));
}

void FPoorforceToolbar::BuildDropdownMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("PoorforceLocks", LOCTEXT("LocksSection", "Locks"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Status", "Status"),
			LOCTEXT("StatusTooltip", "내가 보유한 락 목록 보기 / 해제"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				FGlobalTabmanager::Get()->TryInvokeTab(PoorforceStatusTabId);
			})));
	}
	MenuBuilder.EndSection();
}

TSharedRef<SDockTab> FPoorforceToolbar::SpawnStatusTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SPoorforceLockWindow)
		];
}

#undef LOCTEXT_NAMESPACE
