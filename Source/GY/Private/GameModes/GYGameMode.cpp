#include "GameModes/GYGameMode.h"

#include "AbilitySystemComponent.h"
#include "SkeletalMeshTypes.h"
#include "Character/GYCharacter.h"
#include "Character/GYPawnData.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "Core/GameplayTags/StateTags.h"
#include "Equipment/ActiveEquipmentComponent.h"
#include "Experience/GYExperienceDefinition.h"
#include "Experience/GYExperienceManagerComponent.h"
#include "GameStates/GYGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/GYLogManager.h"
#include "Misc/TrackedActivity.h"
#include "Persistence/CharacterSaveComponent.h"
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
#if UE_BUILD_SHIPPING
	return false;
#else
	return true;
#endif
}

FString AGYGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// 폰 init 체인(GameplayReady)의 EnsureLoaded 보다 먼저 도는 지점 — 로드 시작 전에 id 지정.
	// TODO (KDY): 지금은 클라 신고를 신뢰 — 토큰 게이팅 붙일 때 캐릭터 소유 검증 추가
	const FString CharIdOption = UGameplayStatics::ParseOption(Options, TEXT("charId"));
	if (!CharIdOption.IsEmpty())
	{
		AGYPlayerState* PS = IsValid(NewPlayerController) ? NewPlayerController->GetPlayerState<AGYPlayerState>() : nullptr;
		UCharacterSaveComponent* SaveComponent = IsValid(PS) ? PS->GetCharacterSaveComponent() : nullptr;
		if (IsValid(SaveComponent))
		{
			SaveComponent->SetCharacterId(FCString::Atoi64(*CharIdOption));
			GY_LOG(Network, KDY, "InitNewPlayer: charId=%s assigned to %s", *CharIdOption, *GetNameSafe(PS));
		}
	}

	return Result;
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

	// bGameBGMStarted는 리플리케이트되므로, 이 시점 이후 접속하는 클라이언트도 OnRep으로 자동 재생됨.
	if (AGYGameState* GYGameState = GetGameState<AGYGameState>())
	{
		GYGameState->StartGameBGM();
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

	// PawnData가 없으면 PS에 심을 수 없고, 폰의 PawnExtension이 DataAvailable에서 영구 정지한다(입력/카메라 먹통).
	// 설정 오류(Experience.DefaultPawnData 미지정 등)이므로 스폰 시점에 명확히 경고한다.
	if (!PawnData)
	{
		GY_WARN(Game, KDY, "GetPawnDataForController: PawnData 해결 실패 — Experience.DefaultPawnData 확인 필요. 폰 init이 멈출 수 있음.");
	}

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

	UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent();
	if (!ASC) return;

	ASC->RevokeGrantSource(GYStateTags::State_Life_Dead);

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
