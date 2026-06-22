#include "GameModes/GYGameMode.h"

#include "AbilitySystemComponent.h"
#include "SkeletalMeshTypes.h"
#include "Character/GYCharacter.h"
#include "Character/GYPawnData.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Character/GYPlayerActionConfig.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "Equipment/ActiveEquipmentComponent.h"
#include "Experience/GYExperienceDefinition.h"
#include "Experience/GYExperienceManagerComponent.h"
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

void AGYGameMode::InitGameState()
{
	Super::InitGameState();

	AGYGameState* GYGameState = GetGameState<AGYGameState>();
	if (!IsValid(GYGameState)) return;

	UGYExperienceManagerComponent* ExperienceComponent = GYGameState->GetExperienceManagerComponent();
	if (!IsValid(ExperienceComponent)) return;

	// 로드 완료 콜백을 먼저 등록한 뒤 로드를 시작한다(동기 완료 대비).
	ExperienceComponent->CallOrRegister_OnExperienceLoaded(
		FOnGYExperienceLoaded::FDelegate::CreateUObject(this, &AGYGameMode::OnExperienceLoaded));
	ExperienceComponent->ServerSetCurrentExperience(DefaultExperience);
}

bool AGYGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// Experience 로드 완료 후 OnExperienceLoaded에서 일괄 스폰하므로 시작 지점 자동 스폰을 막는다.
	return false;
}

void AGYGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Experience가 로드되기 전엔 폰 스폰을 보류한다.
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

bool AGYGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return IsExperienceLoaded() && Super::PlayerCanRestart_Implementation(Player);
}

bool AGYGameMode::IsExperienceLoaded() const
{
	const AGYGameState* GYGameState = GetGameState<AGYGameState>();
	if (!IsValid(GYGameState)) return false;

	const UGYExperienceManagerComponent* ExperienceComponent = GYGameState->GetExperienceManagerComponent();
	if (!IsValid(ExperienceComponent)) return false;

	return ExperienceComponent->IsExperienceLoaded();
}

void AGYGameMode::OnExperienceLoaded(const UGYExperienceDefinition* Experience)
{
	// 로드 완료 시점에 폰이 없는 모든 컨트롤러를 스폰한다.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = Cast<APlayerController>(*It);
		if (IsValid(PlayerController) && PlayerController->GetPawn() == nullptr)
		{
			// PawnData는 RestartPlayer 내부의 GetDefaultPawnClassForController에서 PS에 심는다.
			if (PlayerCanRestart(PlayerController))
			{
				RestartPlayer(PlayerController);
			}
		}
	}
}

const UGYPawnData* AGYGameMode::GetPawnDataForController(AController* InController) const
{
	// 이미 PS에 PawnData가 있으면 그것을 우선(리스폰 등 재진입 대응).
	if (InController)
	{
		if (const AGYPlayerState* PS = InController->GetPlayerState<AGYPlayerState>())
		{
			if (const UGYPawnData* ExistingPawnData = PS->GetPawnData())
			{
				return ExistingPawnData;
			}
		}
	}

	// 없으면 현재 Experience의 DefaultPawnData.
	if (const AGYGameState* GYGameState = GetGameState<AGYGameState>())
	{
		if (const UGYExperienceManagerComponent* ExperienceComponent = GYGameState->GetExperienceManagerComponent())
		{
			if (const UGYExperienceDefinition* Experience = ExperienceComponent->GetCurrentExperience())
			{
				return Experience->DefaultPawnData;
			}
		}
	}

	return nullptr;
}

UClass* AGYGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const UGYPawnData* PawnData = GetPawnDataForController(InController);

	// 모든 스폰 경로(HandleStartingNewPlayer / OnExperienceLoaded → RestartPlayer)가 이 함수를 거친다.
	// 폰이 스폰되기 전에 여기서 PS에 PawnData를 심어, 폰의 PawnExtension이 init될 때 항상 준비돼 있게 한다.
	if (PawnData && InController)
	{
		if (AGYPlayerState* PS = InController->GetPlayerState<AGYPlayerState>())
		{
			PS->SetPawnData(PawnData);
		}
	}

	if (PawnData && PawnData->PawnClass)
	{
		return PawnData->PawnClass;
	}

	// PawnData/PawnClass 미지정이면 기본 폰 클래스로 폴백.
	return Super::GetDefaultPawnClassForController_Implementation(InController);
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
			if (const UGYPawnData* PawnData = ExtComp->GetPawnData()) Config = PawnData->ActionConfig;
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
