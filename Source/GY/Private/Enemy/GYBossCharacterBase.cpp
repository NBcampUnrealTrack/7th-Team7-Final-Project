#include "Enemy/GYBossCharacterBase.h"

#include "Enemy/GYBossAIController.h"
#include "Enemy/Component/BossPhaseComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/GameplayTags/StateTags.h"
#include "GameFramework/PlayerState.h"

#include "Net/UnrealNetwork.h"

AGYBossCharacterBase::AGYBossCharacterBase()
{
	PhaseComponent = CreateDefaultSubobject<UBossPhaseComponent>(TEXT("PhaseComponent"));
	PhaseComponent->SetIsReplicated(true);

	AIControllerClass = AGYBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bIsActivate = false;
}

void AGYBossCharacterBase::SetParticipants(const TArray<APlayerState*>& InParticipants)
{
	if (!HasAuthority()) return;

	TArray<TObjectPtr<APlayerState>> NewList;
	NewList.Reserve(InParticipants.Num());
	for (APlayerState* PS : InParticipants)
	{
		if (PS && !NewList.Contains(PS))
		{
			NewList.Add(PS);
		}
	}

	if (bEncounterStarted && NewList.Num() == Participants.Num())
	{
		bool bSame = true;
		for (int32 i = 0; i < NewList.Num(); ++i)
		{
			if (NewList[i] != Participants[i])
			{
				bSame = false;
				break;
			}
			if (bSame) return;
		}
	}

	Participants = MoveTemp(NewList);
	MARK_PROPERTY_DIRTY_FROM_NAME(AGYBossCharacterBase, Participants, this);

	if (!bEncounterStarted)
	{
		bEncounterStarted = true;

		TryGrantGASFromDataAsset();
		Activate();

		if (PhaseComponent)
		{
			if (UBossDataAsset* BossData = GetBossData())
			{
				PhaseComponent->InitializeForEncounter(BossData->PhaseTriggers);
			}
		}
		OnEncounterStarted.Broadcast(Participants.Num());
	}

	OnParticipantCountChanged.Broadcast(Participants.Num());
}

TArray<APlayerState*> AGYBossCharacterBase::GetParticipants() const
{
	TArray<APlayerState*> Result;
	Result.Reserve(Participants.Num());
	for (const TObjectPtr<APlayerState>& PS : Participants)
	{
		if (PS) Result.Add(PS);
	}
	return Result;
}

TArray<APawn*> AGYBossCharacterBase::GetParticipantPawns() const
{
	TArray<APawn*> Result;
	Result.Reserve(Participants.Num());
	for (APlayerState* PS : Participants)
	{
		if (PS)
		{
			if (APawn* Pawn = PS->GetPawn())
			{
				Result.Add(Pawn);
			}
		}
	}
	return Result;
}

void AGYBossCharacterBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AGYBossCharacterBase, Participants, Params);

}

float AGYBossCharacterBase::GetStatScaleValue() const
{
	return static_cast<float>(FMath::Max(1, Participants.Num()));
}

void AGYBossCharacterBase::OnRep_Participants()
{
	OnParticipantCountChanged.Broadcast(Participants.Num());
}

void AGYBossCharacterBase::HandleStaggerBegin()
{
	Super::HandleStaggerBegin();

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UStateTreeAIComponent* ST = AI->FindComponentByClass<UStateTreeAIComponent>())
		{
			FStateTreeEvent Event;
			Event.Tag = GYStateTags::State_Hit_Stagger;
			ST->SendStateTreeEvent(Event);
		}
	}
}

void AGYBossCharacterBase::HandleStunBegin()
{
	Super::HandleStunBegin();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UStateTreeAIComponent* ST = AI->FindComponentByClass<UStateTreeAIComponent>())
		{
			FStateTreeEvent Event;
			Event.Tag = GYStateTags::State_Hit_Stun;
			ST->SendStateTreeEvent(Event);
		}
	}
}

void AGYBossCharacterBase::Die()
{
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UStateTreeAIComponent* ST = AI->FindComponentByClass<UStateTreeAIComponent>())
		{
			FStateTreeEvent Event;
			Event.Tag = GYStateTags::State_Life_Dead;
			ST->SendStateTreeEvent(Event);
		}
	}

	Super::Die();
}
