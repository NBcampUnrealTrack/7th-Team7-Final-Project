#include "Enemy/GYBossCharacterBase.h"

#include "Enemy/GYBossAIController.h"
#include "Enemy/Component/BossPhaseComponent.h"

#include "Net/UnrealNetwork.h"

AGYBossCharacterBase::AGYBossCharacterBase()
{
	PhaseComponent = CreateDefaultSubobject<UBossPhaseComponent>(TEXT("PhaseComponent"));
	PhaseComponent->SetIsReplicated(true);

	AIControllerClass = AGYBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bIsActivate = false;
}

void AGYBossCharacterBase::SetParticipantCount(int32 NewCount)
{
	if (!HasAuthority()) return;

	NewCount = FMath::Max(1, NewCount);
	if (ParticipantCount == NewCount && bEncounterStarted) return;

	ParticipantCount = NewCount;
	MARK_PROPERTY_DIRTY_FROM_NAME(AGYBossCharacterBase, ParticipantCount, this);

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
				if (AGYBossAIController* AI = Cast<AGYBossAIController>(GetController()))
				{
					if (UBossPatternSelectorComponent* Selector = AI->GetPatternSelector())
					{
						Selector->InitializePatterns(BossData->NormalPatterns);
					}
				}
			}
			else
			{
				PhaseComponent->InitializeForEncounter(TArray<FBossPhaseTrigger>());
			}
		}
		OnEncounterStarted.Broadcast(ParticipantCount);
	}
	else
	{
		//TODO 은서 : 플레이어 수를 동적으로 받는경우 추가 필요
	}
	OnParticipantCountChanged.Broadcast(ParticipantCount);
}

void AGYBossCharacterBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(AGYBossCharacterBase, ParticipantCount, Params);

}

float AGYBossCharacterBase::GetStatScaleValue() const
{
	return static_cast<float>(FMath::Max(1, ParticipantCount));
}

void AGYBossCharacterBase::OnRep_ParticipantCount()
{
	OnParticipantCountChanged.Broadcast(ParticipantCount);
}
