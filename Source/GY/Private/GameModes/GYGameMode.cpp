#include "GameModes/GYGameMode.h"

#include "Character/GYCharacter.h"
#include "Player/GYPlayerController.h"
#include "Player/GYPlayerState.h"

AGYGameMode::AGYGameMode()
{
	DefaultPawnClass = AGYCharacter::StaticClass();
	PlayerStateClass = AGYPlayerState::StaticClass();
	PlayerControllerClass = AGYPlayerController::StaticClass();
}
