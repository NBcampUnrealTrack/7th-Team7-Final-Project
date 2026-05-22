#include "GameModes/GYGameMode.h"

#include "Character/GYCharacter.h"
#include "GameStates/GYGameState.h"
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

void AGYGameMode::UpdateWorldTime(float DeltaTime)
{
	if (false == HasAuthority()) return;

	AGYGameState* GYGameState = GetGameState<AGYGameState>();
	if (false == IsValid(GYGameState)) return;

	const UGYWorldDataSettings* ActorGuidDataSettings = GetDefault<UGYWorldDataSettings>();
	if (!ActorGuidDataSettings) return;

	float CurrentTime = GYGameState->GetCurrentTime();

	CurrentTime += DeltaTime * GYGameState->GetTimeScale();
	GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, FString::Printf(TEXT("CurrentTIme : %f"), CurrentTime));
	if (CurrentTime > GYGameState->GetMidnight())
	{
		UGYWorldResetSubsystem* WorldResetSubsystem = GetGameInstance()->GetSubsystem<UGYWorldResetSubsystem>();
		if (WorldResetSubsystem)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, FString::Printf(TEXT("WorldReset!")));
			WorldResetSubsystem->ResetWorld();
		}
		CurrentTime = ActorGuidDataSettings->StartOfDayHour * 60.f * 60.f;
	}
	GYGameState->SetCurrentTime(CurrentTime);

}
