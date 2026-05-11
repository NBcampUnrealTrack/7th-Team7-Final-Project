#include "GameModes/GYGameMode.h"

#include "Character/GYCharacter.h"
#include "Player/GYPlayerState.h"

AGYGameMode::AGYGameMode()
{
	DefaultPawnClass = AGYCharacter::StaticClass();
	PlayerStateClass = AGYPlayerState::StaticClass();
}
