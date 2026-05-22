#include "Player/GYPlayerController.h"
#include "Cheats/GYCheatManager.h"
#include "GYUI/Public/Core/GYPrimaryGameLayout.h"
#include "GYUI/Public/Core/GYUIManagerSubsystem.h"
#include "GYUI/Public/GameplayTags/GYUILayerTags.h"
#include "CommonActivatableWidget.h"
#include "Character/GYHeroComponent.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Cheats/GYServerCheatProxy.h"
#include "Net/UnrealNetwork.h"

AGYPlayerController::AGYPlayerController()
{
	CheatClass = UGYCheatManager::StaticClass();
}

void AGYPlayerController::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING
	if (HasAuthority())
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		ServerCheatProxy = GetWorld()->SpawnActor<AGYServerCheatProxy>(
			ServerCheatProxyClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

		if (ServerCheatProxy)
		{
			ServerCheatProxy->OwnerController = this;
		}
	}
#endif
	// 안정성 체크 -> UI 서버 생성 차단
	if (IsLocalController())
	{
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
		EnableCheats();
	}
}

void AGYPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

#if !UE_BUILD_SHIPPING
	DOREPLIFETIME(AGYPlayerController,ServerCheatProxy);
#endif
}

void AGYPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

