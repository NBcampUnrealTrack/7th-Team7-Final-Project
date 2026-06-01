#include "GameModes/GYGameMode.h"

#include "SkeletalMeshTypes.h"
#include "Character/GYCharacter.h"
#include "GameStates/GYGameState.h"
#include "Misc/TrackedActivity.h"
#include "Player/GYPlayerController.h"
#include "Player/GYPlayerState.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"

AGYGameMode::AGYGameMode()
{
	DefaultPawnClass = AGYCharacter::StaticClass();
	PlayerStateClass = AGYPlayerState::StaticClass();
	PlayerControllerClass = AGYPlayerController::StaticClass();
	GameStateClass = AGYGameState::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

bool AGYGameMode::AllowCheats(APlayerController* P)
{
	return true;
}

void AGYGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateWorldTime(DeltaSeconds);

}

bool AGYGameMode::AdvanceSecond(float Amount, AGYGameState* GYGameState)
{
	const UGYWorldDataSettings* ActorGuidDataSettings = GetDefault<UGYWorldDataSettings>();
	if (!ActorGuidDataSettings) return true;

	bool ret = false;
	float CurrentTime = GYGameState->GetCurrentTime();
	GEngine->AddOnScreenDebugMessage(-1,1,FColor::Red,FString::FromInt(CurrentTime));

	CurrentTime += Amount;
	if (CurrentTime > GYGameState->GetMidnight())
	{
		UGYWorldResetSubsystem* WorldResetSubsystem = GetGameInstance()->GetSubsystem<UGYWorldResetSubsystem>();
		if (WorldResetSubsystem)
		{
			WorldResetSubsystem->ResetWorld();
			ret = true;
		}
		CurrentTime = ActorGuidDataSettings->StartOfDayHour * 60.f * 60.f;
	}
	GYGameState->SetCurrentTime(CurrentTime);

	return ret;
}

bool AGYGameMode::UpdateWorldTime(float DeltaTime)
{
	AGYGameState* GYGameState = GetGameState<AGYGameState>();
	if (false == IsValid(GYGameState)) return false;

	float Amount = DeltaTime * GYGameState->GetTimeScale();


	return AdvanceSecond(Amount, GYGameState);

}

bool AGYGameMode::AdvanceHour(float Hour)
{
	AGYGameState* GYGameState = GetGameState<AGYGameState>();
	if (false == IsValid(GYGameState)) return false;
	float Amount = Hour*60.f*60.f;
	return AdvanceSecond(Amount, GYGameState);
}
