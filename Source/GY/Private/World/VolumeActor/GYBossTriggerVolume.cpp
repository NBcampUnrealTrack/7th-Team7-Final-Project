#include "World/VolumeActor/GYBossTriggerVolume.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

AGYBossTriggerVolume::AGYBossTriggerVolume()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
}

void AGYBossTriggerVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYBossTriggerVolume, ResolvedBoss);
}

void AGYBossTriggerVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ServerResolveTimer);
		World->GetTimerManager().ClearTimer(ShowRetryTimer);
	}
	HideBoss();
	Super::EndPlay(EndPlayReason);
}

bool AGYBossTriggerVolume::IsLocalPlayerPawn(const APawn* Pawn) const
{
	return Pawn && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled();
}

void AGYBossTriggerVolume::HandlePawnEntered(APawn* Pawn)
{
	if (!Pawn || !Pawn->IsPlayerControlled()) return;
	if (HasAuthority())
	{
		ServerTryResolveBoss();
	}

	if (Pawn->IsLocallyControlled())
	{
		bLocalPlayerInside = true;
		ShowRetryCount = 0;
		TryShowBossLocal();
	}
}

void AGYBossTriggerVolume::HandlePawnExited(APawn* Pawn)
{
	if (!IsLocalPlayerPawn(Pawn)) return;

	bLocalPlayerInside = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShowRetryTimer);
	}
	HideBoss();
}

void AGYBossTriggerVolume::ServerTryResolveBoss()
{
	if (!HasAuthority()) return;
	if (ResolvedBoss)   return;

	if (AGYEnemyCharacterBase* Boss = TargetBoss.Get())
	{
		ResolvedBoss = Boss;
		ForceNetUpdate();
		OnRep_ResolvedBoss();
		return;
	}

	// 서버에도 아직 스트리밍 전 - 재시도
	UWorld* World = GetWorld();
	if (!World) return;

	if (ServerResolveRetryCount < ResolveRetryMaxCount)
	{
		++ServerResolveRetryCount;
		World->GetTimerManager().SetTimer(ServerResolveTimer,
			FTimerDelegate::CreateUObject(this, &AGYBossTriggerVolume::ServerTryResolveBoss),
			ResolveRetryInterval, false);
	}
}

void AGYBossTriggerVolume::OnRep_ResolvedBoss()
{
	// 보스가 복제로 도착 - 로컬 플레이어가 안에 있으면 표시
	if (bLocalPlayerInside)
	{
		TryShowBossLocal();
	}
}

void AGYBossTriggerVolume::TryShowBossLocal()
{
	if (!bLocalPlayerInside) return;
	if (bShown) return;

	AGYEnemyCharacterBase* Boss = ResolvedBoss;
	if (Boss)
	{
		if (Boss->IsDead()) return; // 이미 죽은 보스면 표시하지 않음
		ShowBoss(Boss);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	if (ShowRetryCount < ResolveRetryMaxCount)
	{
		++ShowRetryCount;
		World->GetTimerManager().SetTimer(ShowRetryTimer,
			FTimerDelegate::CreateUObject(this, &AGYBossTriggerVolume::TryShowBossLocal),
			ResolveRetryInterval, false);
	}
}

void AGYBossTriggerVolume::ShowBoss(AGYEnemyCharacterBase* Boss)
{
	if (!Boss) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FGYBossStateMessage State;
	State.bVisible   = true;
	State.TargetBoss = Boss;

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Boss_State, State);
	bShown = true;
}

void AGYBossTriggerVolume::HideBoss()
{
	ShowRetryCount = 0;

	if (!bShown) return;
	bShown = false;

	UWorld* World = GetWorld();
	if (!World) return;

	FGYBossStateMessage State;
	State.bVisible   = false;
	State.TargetBoss = nullptr;

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Boss_State, State);
}
