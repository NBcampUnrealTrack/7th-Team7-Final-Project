#include "ItemEditor/GYItemEditorRegistration.h"

#include "ItemEditor/SGYItemEditorWindow.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

namespace
{
	const FName GYItemEditorTabId("GYItemEditor");
	const FName GYMenuOwner("GYItemEditor");
}

void FGYItemEditorRegistration::Register()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			GYItemEditorTabId,
			FOnSpawnTab::CreateStatic(&FGYItemEditorRegistration::SpawnItemEditorTab))
		.SetDisplayName(LOCTEXT("TabTitle", "아이템 에디터"))
		.SetTooltipText(LOCTEXT("TabTooltip", "아이템 정의·스탯·드랍 설정을 한 화면에서 편집합니다"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateStatic(&FGYItemEditorRegistration::RegisterMenus));
}

void FGYItemEditorRegistration::Unregister()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GYItemEditorTabId);
	}

	// 엔진 종료 중엔 UToolMenus::Get()이 nullptr — 가드 필수
	if (!UObjectInitialized() || IsEngineExitRequested()) return;

	if (UToolMenus* ToolMenus = UToolMenus::Get())
	{
		ToolMenus->UnregisterOwnerByName(GYMenuOwner);
	}
}

void FGYItemEditorRegistration::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(GYMenuOwner);

	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
	FToolMenuSection& Section = MainMenu->AddSection("GY");

	Section.AddSubMenu(
		"GYMenu",
		LOCTEXT("GYMenuLabel", "GY"),
		LOCTEXT("GYMenuTooltip", "GY 프로젝트 툴"),
		FNewMenuDelegate::CreateStatic(&FGYItemEditorRegistration::BuildGYMenu));
}

void FGYItemEditorRegistration::BuildGYMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenItemEditor", "아이템 에디터"),
		LOCTEXT("OpenItemEditorTooltip", "아이템 정의·스탯·드랍 설정을 한 화면에서 편집합니다"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(GYItemEditorTabId);
		})));
}

TSharedRef<SDockTab> FGYItemEditorRegistration::SpawnItemEditorTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SGYItemEditorWindow)
		];
}

#undef LOCTEXT_NAMESPACE
