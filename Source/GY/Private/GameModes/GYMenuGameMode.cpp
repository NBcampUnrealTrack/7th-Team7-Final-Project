#include "GameModes/GYMenuGameMode.h"

#include "Core/GameplayTags/SoundTags.h"
#include "Core/Sound/GYSoundManager.h"
#include "Player/GYPlayerController.h"

AGYMenuGameMode::AGYMenuGameMode()
{
	PlayerControllerClass = AGYPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}

void AGYMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() == NM_DedicatedServer) return;

	if (UGYSoundManager* SoundMgr = UGYSoundManager::Get(this))
	{
		SoundMgr->PlayBGM(GYGameplayTags::Sound_BGM_Lobby, 0.5f);
	}
}
