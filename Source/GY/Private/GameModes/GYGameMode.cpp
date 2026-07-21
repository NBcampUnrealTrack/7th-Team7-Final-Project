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
#include "WorldSession/GYWorldSessionSubsystem.h"
#include "Persistence/WorldSaveComponent.h"
#include "Player/GYPlayerController.h"
#include "Player/GYPlayerState.h"
#include "World/ActorManagement/GYRespawnStreamingSource.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"
#include "WorldGimmick/TimeRift/TimeRiftSubsystem.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

namespace
{
	constexpr float GRespawnStreamingPollInterval = 1.f;
	constexpr float GRespawnStreamingMaxExtraWait = 15.f;
}

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

void AGYGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	constexpr int32 MaxPlayersPerWorld = 4;

	const AGameStateBase* CurrentGameState = GetGameState<AGameStateBase>();
	if (IsValid(CurrentGameState) && CurrentGameState->PlayerArray.Num() >= MaxPlayersPerWorld)
	{
		GY_WARN(Network, KDY, "PreLogin rejected from %s - world full (%d/%d)", *Address, CurrentGameState->PlayerArray.Num(), MaxPlayersPerWorld);
		ErrorMessage = TEXT("world_full");
		return;
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void AGYGameMode::Logout(AController* Exiting)
{
	if (APlayerController* ExitingPC = Cast<APlayerController>(Exiting))
	{
		CleanupPendingRespawn(ExitingPC);
	}

	if (APawn* OldPawn = Exiting->GetPawn())
	{
		Exiting->UnPossess();
		OldPawn->Destroy();
	}
	Super::Logout(Exiting);

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

		// 참여자 기록 — 클라 "참가 중인 월드" 분류의 근거 (월드 영속이 있는 서버만)
		const AGYGameState* GYGameState = GetGameState<AGYGameState>();
		const UWorldSaveComponent* WorldSave = IsValid(GYGameState) ? GYGameState->GetWorldSaveComponent() : nullptr;
		if (IsValid(WorldSave) && WorldSave->IsPersistenceEnabled())
		{
			UGYWorldSessionSubsystem* Session = GetGameInstance()->GetSubsystem<UGYWorldSessionSubsystem>();
			if (IsValid(Session))
			{
				Session->RecordWorldParticipant(WorldSave->GetWorldId(), FCString::Atoi64(*CharIdOption));
			}
		}
	}

	return Result;
}

bool AGYGameMode::FindInactivePlayer(APlayerController* PC)
{
	return false;
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
	// Experience 와 월드 상태가 준비되기 전엔 폰 스폰을 보류한다.
	if (IsExperienceLoaded() && IsWorldStateReady())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

bool AGYGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return IsExperienceLoaded() && IsWorldStateReady() && Super::PlayerCanRestart_Implementation(Player);
}

void AGYGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);

	AGYPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<AGYPlayerState>() : nullptr;
	APawn* Pawn = NewPlayer ? NewPlayer->GetPawn() : nullptr;
	if (PS && Pawn && !PS->HasInitialSpawnTransform())
	{
		PS->SetInitialSpawnTransform(Pawn->GetActorTransform());
	}
}

bool AGYGameMode::IsExperienceLoaded() const
{
	const AGYGameState* GYGameState = GetGameState<AGYGameState>();
	if (!IsValid(GYGameState)) return false;

	const UGYExperienceManagerComponent* ExperienceComponent = GYGameState->GetExperienceManagerComponent();
	if (!IsValid(ExperienceComponent)) return false;

	return ExperienceComponent->IsExperienceLoaded();
}

bool AGYGameMode::IsWorldStateReady() const
{
	const AGYGameState* GYGameState = GetGameState<AGYGameState>();
	if (!IsValid(GYGameState)) return true;

	const UWorldSaveComponent* WorldSave = GYGameState->GetWorldSaveComponent();
	if (!IsValid(WorldSave) || !WorldSave->IsPersistenceEnabled()) return true;

	return WorldSave->IsWorldStateReady();
}

void AGYGameMode::OnWorldStateReady()
{
	// Experience 게이트와 동일한 후처리 — 두 게이트 중 늦게 열리는 쪽이 스폰을 트리거한다
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = Cast<APlayerController>(*It);
		if (IsValid(PlayerController) && PlayerController->GetPawn() == nullptr)
		{
			if (PlayerCanRestart(PlayerController))
			{
				RestartPlayer(PlayerController);
			}
		}
	}
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

	CleanupPendingRespawn(PC);

	const FTransform SpawnTransform = ResolveRespawnTransform(PC);

	FGYPendingRespawn& Pending = PendingRespawns.Add(PC);
	Pending.SpawnTransform = SpawnTransform;
	Pending.ExtraWaitElapsed = 0.f;

	if (UWorldPartitionSubsystem* WPSubsystem = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>())
	{
		Pending.StreamingSource = MakeShared<FGYRespawnStreamingSource>(SpawnTransform.GetLocation(), SpawnTransform.Rotator());
		WPSubsystem->RegisterStreamingSourceProvider(Pending.StreamingSource.Get());
	}

	if (AGYPlayerController* GYPC = Cast<AGYPlayerController>(PC))
	{
		GYPC->Client_PrewarmRespawnStreaming(SpawnTransform.GetLocation(), SpawnTransform.Rotator());
	}

	FTimerHandle Handle;
	FTimerDelegate Del;
	Del.BindUObject(this, &AGYGameMode::TryPerformRespawn, TWeakObjectPtr<APlayerController>(PC));
	GetWorldTimerManager().SetTimer(Handle, Del, Delay, false);
}

FTransform AGYGameMode::ResolveRespawnTransform(APlayerController* PC) const
{
	const AGYPlayerState* PS = PC ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	if (!PS) return FTransform::Identity;

	FTransform SpawnTransform = PS->GetInitialSpawnTransform();

	if (UTimeRiftSubsystem* TimeRiftSubsystem = GetGameInstance()->GetSubsystem<UTimeRiftSubsystem>())
	{
		TimeRiftSubsystem->TryGetRespawnTransform(PS->GetLastCheckpointId(), SpawnTransform);
	}

	return SpawnTransform;
}

void AGYGameMode::TryPerformRespawn(TWeakObjectPtr<APlayerController> WeakPC)
{
	APlayerController* PC = WeakPC.Get();
	FGYPendingRespawn* Pending = PC ? PendingRespawns.Find(PC) : nullptr;
	if (!PC || !Pending)
	{
		return;
	}

	UWorldPartitionSubsystem* WPSubsystem = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>();
	const bool bStreamingReady = !WPSubsystem || WPSubsystem->IsStreamingCompleted(Pending->StreamingSource.Get());
	const bool bTimedOut = Pending->ExtraWaitElapsed >= GRespawnStreamingMaxExtraWait;

	if (!bStreamingReady && !bTimedOut)
	{
		Pending->ExtraWaitElapsed += GRespawnStreamingPollInterval;

		FTimerHandle Handle;
		FTimerDelegate Del;
		Del.BindUObject(this, &AGYGameMode::TryPerformRespawn, WeakPC);
		GetWorldTimerManager().SetTimer(Handle, Del, GRespawnStreamingPollInterval, false);
		return;
	}

	const FTransform SpawnTransform = Pending->SpawnTransform;
	CleanupPendingRespawn(PC);
	PerformRespawn(PC, SpawnTransform);
}

void AGYGameMode::CleanupPendingRespawn(APlayerController* PC)
{
	if (!PC) return;

	if (FGYPendingRespawn* Pending = PendingRespawns.Find(PC))
	{
		if (UWorldPartitionSubsystem* WPSubsystem = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>())
		{
			WPSubsystem->UnregisterStreamingSourceProvider(Pending->StreamingSource.Get());
		}
		PendingRespawns.Remove(PC);
	}

	if (AGYPlayerController* GYPC = Cast<AGYPlayerController>(PC))
	{
		GYPC->Client_ClearRespawnPrewarm();
	}
}

void AGYGameMode::PerformRespawn(APlayerController* PC, const FTransform& SpawnTransform)
{
	if (!PC) return;

	AGYPlayerState* PS = PC->GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent();
	if (!ASC) return;

	ASC->RevokeGrantSource(GYStateTags::State_Life_Dead);

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
