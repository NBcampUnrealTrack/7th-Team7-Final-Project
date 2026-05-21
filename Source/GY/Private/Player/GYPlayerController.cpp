#include "Player/GYPlayerController.h"
#include "Cheats/GYCheatManager.h"
#include "GYUI/Public/Core/GYPrimaryGameLayout.h"
#include "GYUI/Public/Core/GYUIManagerSubsystem.h"
#include "GYUI/Public/GameplayTags/GYUILayerTags.h"
#include "CommonActivatableWidget.h"


AGYPlayerController::AGYPlayerController()
{
	CheatClass = UGYCheatManager::StaticClass();
}

void AGYPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 안정성 체크 -> UI 서버 생성 차단
	if (!IsLocalController()) return;

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer) return;

	UGYUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UGYUIManagerSubsystem>();
	if (!UIManager) return;

	if (PrimaryGameLayoutClass) // 레이아웃을 전체 화면에 띄움
	{
		UIManager->CreatePrimaryGameLayout(PrimaryGameLayoutClass);
	}

	if (HUDWidgetClass) // Layer_Game에 HUD 넣어두기
	{
		UIManager->PushWidgetToLayer(GYUILayerTags::UI_Layer_Game, HUDWidgetClass);
	}
}

void AGYPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

