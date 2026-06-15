#include "GameModes/GYGameMode.h"

#include "AbilitySystemComponent.h"
#include "SkeletalMeshTypes.h"
#include "Character/GYCharacter.h"
#include "Character/GYPawnData.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Character/GYPlayerActionConfig.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "Equipment/ActiveEquipmentComponent.h"
#include "GameStates/GYGameState.h"
#include "Misc/TrackedActivity.h"
#include "Player/GYPlayerController.h"
#include "Player/GYPlayerState.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"
#include "WorldGimmick/TimeRift/TimeRiftSubsystem.h"

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

void AGYGameMode::RequestRespawn(APlayerController* PC, float Delay)
{
	if (!PC) return;
	FTimerHandle Handle;
	FTimerDelegate Del;
	Del.BindUObject(this, &AGYGameMode::PerformRespawn, PC);
	GetWorldTimerManager().SetTimer(Handle, Del, Delay, false);
}

void AGYGameMode::PerformRespawn(APlayerController* PC)
{
	if (!PC) return;

	AGYPlayerState* PS = PC->GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;

	const UGYPlayerActionConfig* Config = nullptr;
	if (APawn* OldPawn = PC->GetPawn())
	{
		if (UGYPawnExtensionComponent* ExtComp = OldPawn->FindComponentByClass<UGYPawnExtensionComponent>())
		{
			if (ExtComp->PawnData) Config = ExtComp->PawnData->ActionConfig;
		}
	}

	if (Config)
	{
		for (const FGameplayTag& Tag : Config->DeathTags)
		{
			ASC->RemoveLooseGameplayTag(Tag, 1, EGameplayTagReplicationState::TagOnly);
		}
	}

	FTransform SpawnTransform = FTransform::Identity;

	if (UTimeRiftSubsystem* TimeRiftSubsystem = GetGameInstance()->GetSubsystem<UTimeRiftSubsystem>())
	{
		TimeRiftSubsystem->TryGetRespawnTransform(PS->GetLastCheckpointId(), SpawnTransform);
	}

	if (APawn* OldPawn = PC->GetPawn())
	{
		if (UActiveEquipmentComponent* Equip = OldPawn->FindComponentByClass<UActiveEquipmentComponent>())
		{
			Equip->MulticastRemoveAllVisuals();
		}

		PC->UnPossess();
		OldPawn->Destroy();
	}

	RestartPlayerAtTransform(PC, SpawnTransform);

	const float MaxHP = ASC->GetNumericAttribute(UGYPlayerVitalAttributeSet::GetMaxHealthAttribute());
	ASC->SetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetCurrentHealthAttribute(), MaxHP);
}
