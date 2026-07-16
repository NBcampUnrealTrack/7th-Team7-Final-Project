#include "Persistence/WorldSessionComponent.h"

#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"
#include "Persistence/WorldSaveComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

namespace
{
	// 월드 목록의 온라인 판정(stale 기준)과 짝 — 오케스트레이터는 90초 무응답을 죽은 세션으로 본다
	constexpr float HeartbeatIntervalSeconds = 30.f;

	// 배정 감지 지연 = 입장 대기 시간에 직결 — 짧게
	constexpr float StandbyPollSeconds = 2.f;
	constexpr float AwaitReadyPollSeconds = 0.5f;
}

UWorldSessionComponent::UWorldSessionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UWorldSessionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority()) return;
	if (!FParse::Value(FCommandLine::Get(), TEXT("PublicAddr="), PublicAddr) || PublicAddr.IsEmpty()) return;

	bStandby = FParse::Param(FCommandLine::Get(), TEXT("Standby"));
	FParse::Value(FCommandLine::Get(), TEXT("WorldId="), WorldId);
	bEnabled = true;

	UGYPersistenceSubsystem* Persistence = ResolvePersistence();
	if (!IsValid(Persistence)) return;

	if (bStandby)
	{
		Persistence->RegisterStandby(PublicAddr,
			FGYOnSaveComplete::CreateUObject(this, &UWorldSessionComponent::OnStandbyRegistered));
		return;
	}

	StartWorldHeartbeat();
}

void UWorldSessionComponent::StartWorldHeartbeat()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	World->GetTimerManager().SetTimer(
		HeartbeatTimerHandle,
		this,
		&UWorldSessionComponent::OnHeartbeatTimer,
		HeartbeatIntervalSeconds,
		true,
		0.f // 첫 하트비트 즉시 — 이게 월드 목록의 online 전환
	);

	GY_LOG(Network, KDY, "WorldSession heartbeat started (worldId=%lld addr=%s)", WorldId, *PublicAddr);
}

void UWorldSessionComponent::OnStandbyRegistered(const FGYSaveResult& Result)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	if (Result.Result != EGYPersistResult::Success || Result.NewVersion <= 0)
	{
		// 등록 실패 — 스탠바이로서 무용하지만 재시도로 회복 (오케스트레이터 리핑과 무관하게 프로세스는 유지)
		GY_WARN(Network, KDY, "Standby register failed - retrying");
		World->GetTimerManager().SetTimer(StandbyPollTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				UGYPersistenceSubsystem* Persistence = ResolvePersistence();
				if (IsValid(Persistence))
				{
					Persistence->RegisterStandby(PublicAddr,
						FGYOnSaveComplete::CreateUObject(this, &UWorldSessionComponent::OnStandbyRegistered));
				}
			}), 5.f, false);
		return;
	}

	StandbyId = Result.NewVersion;
	GY_LOG(Network, KDY, "Standby registered (id=%lld addr=%s) - polling for assignment", StandbyId, *PublicAddr);

	World->GetTimerManager().SetTimer(StandbyPollTimerHandle, this,
		&UWorldSessionComponent::OnStandbyPollTimer, StandbyPollSeconds, true);

	// 스탠바이 생존 신고 (죽은 스탠바이는 오케스트레이터가 행 리핑)
	World->GetTimerManager().SetTimer(HeartbeatTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UGYPersistenceSubsystem* Persistence = ResolvePersistence();
			if (IsValid(Persistence) && StandbyId > 0)
			{
				Persistence->StandbyHeartbeat(StandbyId);
			}
		}), HeartbeatIntervalSeconds, true);
}

void UWorldSessionComponent::OnStandbyPollTimer()
{
	if (bAssignmentPollInFlight || StandbyId <= 0) return;

	UGYPersistenceSubsystem* Persistence = ResolvePersistence();
	if (!IsValid(Persistence)) return;

	bAssignmentPollInFlight = true;
	Persistence->PollStandbyAssignment(StandbyId,
		FGYOnSaveComplete::CreateUObject(this, &UWorldSessionComponent::OnStandbyAssignment));
}

void UWorldSessionComponent::OnStandbyAssignment(const FGYSaveResult& Result)
{
	bAssignmentPollInFlight = false;

	if (Result.Result != EGYPersistResult::Success || Result.NewVersion <= 0) return; // 미배정 — 계속 폴링

	WorldId = Result.NewVersion;
	GY_LOG(Network, KDY, "Standby assigned worldId=%lld - loading save", WorldId);

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;
	World->GetTimerManager().ClearTimer(StandbyPollTimerHandle);
	World->GetTimerManager().ClearTimer(HeartbeatTimerHandle); // 스탠바이 하트비트 중단

	// 세이브 로드 시작 — 완료(입장 게이트 오픈)를 기다렸다가 월드 하트비트로 전환
	AGYGameState* GameState = Cast<AGYGameState>(GetOwner());
	UWorldSaveComponent* WorldSave = IsValid(GameState) ? GameState->GetWorldSaveComponent() : nullptr;
	if (IsValid(WorldSave))
	{
		WorldSave->AssignWorld(WorldId);
	}

	World->GetTimerManager().SetTimer(AwaitReadyTimerHandle, this,
		&UWorldSessionComponent::OnAwaitWorldReadyTimer, AwaitReadyPollSeconds, true);
}

void UWorldSessionComponent::OnAwaitWorldReadyTimer()
{
	const AGYGameState* GameState = Cast<AGYGameState>(GetOwner());
	const UWorldSaveComponent* WorldSave = IsValid(GameState) ? GameState->GetWorldSaveComponent() : nullptr;
	if (!IsValid(WorldSave) || !WorldSave->IsWorldStateReady()) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;
	World->GetTimerManager().ClearTimer(AwaitReadyTimerHandle);

	// 배정 소화 완료 — 스탠바이 행 제거 후 월드 세션으로 전환 (첫 하트비트가 online 전환)
	UGYPersistenceSubsystem* Persistence = ResolvePersistence();
	if (IsValid(Persistence) && StandbyId > 0)
	{
		Persistence->ConsumeStandby(StandbyId);
		StandbyId = 0;
	}
	StartWorldHeartbeat();
}

void UWorldSessionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bEnabled)
	{
		UGYPersistenceSubsystem* Persistence = ResolvePersistence();
		if (IsValid(Persistence))
		{
			if (StandbyId > 0)
			{
				Persistence->ConsumeStandby(StandbyId); // 미배정 종료 — 행 정리
			}
			else if (!bStandby || WorldId > 0)
			{
				Persistence->SetWorldOffline(WorldId);
			}
		}

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
			World->GetTimerManager().ClearTimer(StandbyPollTimerHandle);
			World->GetTimerManager().ClearTimer(AwaitReadyTimerHandle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UWorldSessionComponent::OnHeartbeatTimer()
{
	UGYPersistenceSubsystem* Persistence = ResolvePersistence();
	if (!IsValid(Persistence)) return;

	AGameStateBase* GameState = Cast<AGameStateBase>(GetOwner());
	const int32 PlayerCount = IsValid(GameState) ? GameState->PlayerArray.Num() : 0;
	Persistence->HeartbeatWorld(WorldId, PublicAddr, PlayerCount);
}

UGYPersistenceSubsystem* UWorldSessionComponent::ResolvePersistence() const
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = IsValid(World) ? World->GetGameInstance() : nullptr;
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYPersistenceSubsystem>() : nullptr;
}
