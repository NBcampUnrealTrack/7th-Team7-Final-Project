#include "Player/GYPlayerController.h"
#include "Cheats/GYCheatManager.h"


AGYPlayerController::AGYPlayerController()
{
	CheatClass = UGYCheatManager::StaticClass();
}

void AGYPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

