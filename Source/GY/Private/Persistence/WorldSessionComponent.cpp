#include "Persistence/WorldSessionComponent.h"

#include "Logging/GYLogManager.h"
#include "Persistence/GYPersistenceSubsystem.h"

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

	FParse::Value(FCommandLine::Get(), TEXT("WorldId="), WorldId);
	bEnabled = true;

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

void UWorldSessionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bEnabled)
	{
		UWorld* World = GetWorld();
		UGameInstance* GameInstance = IsValid(World) ? World->GetGameInstance() : nullptr;
		UGYPersistenceSubsystem* Persistence = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYPersistenceSubsystem>() : nullptr;
		if (IsValid(Persistence))
		{
			Persistence->SetWorldOffline(WorldId);
		}

		if (IsValid(World))
		{
			World->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UWorldSessionComponent::OnHeartbeatTimer()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = IsValid(World) ? World->GetGameInstance() : nullptr;
	UGYPersistenceSubsystem* Persistence = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYPersistenceSubsystem>() : nullptr;
	if (!IsValid(Persistence)) return;

	AGameStateBase* GameState = Cast<AGameStateBase>(GetOwner());
	const int32 PlayerCount = IsValid(GameState) ? GameState->PlayerArray.Num() : 0;
	Persistence->HeartbeatWorld(WorldId, PublicAddr, PlayerCount);
}
