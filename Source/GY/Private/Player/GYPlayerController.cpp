#include "Player/GYPlayerController.h"
#include "Cheats/GYCheatManager.h"
#include "Character/GYHeroComponent.h"
#include "Character/GYPawnExtensionComponent.h"

AGYPlayerController::AGYPlayerController()
{
	CheatClass = UGYCheatManager::StaticClass();
}

void AGYPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AGYPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 이 시점에 Controller->PlayerState가 세팅됨
	// HeroComp의 bHasControllerPairedWithPS 조건이 이제 통과 가능
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	if (UGYPawnExtensionComponent* ExtComp = ControlledPawn->FindComponentByClass<UGYPawnExtensionComponent>())
	{
		ExtComp->CheckDefaultInitialization();
	}
	if (UGYHeroComponent* HeroComp = ControlledPawn->FindComponentByClass<UGYHeroComponent>())
	{
		HeroComp->CheckDefaultInitialization();
	}
}
