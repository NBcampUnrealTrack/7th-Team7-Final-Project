#include "GameModes/GYMenuGameMode.h"

#include "Player/GYPlayerController.h"

AGYMenuGameMode::AGYMenuGameMode()
{
	PlayerControllerClass = AGYPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}
