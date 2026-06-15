#include "Enemy/GYBossCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "Enemy/GYBossAIController.h"
#include "Enemy/EnemyAnimInstance.h"
#include "Enemy/Abilities/GYBossPhaseAbility.h"
#include "Enemy/Component/BossPhaseComponent.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/GameplayTags/StateTags.h"
#include "Engine/AssetManager.h"
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

void AGYBossCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	// TODO: 인카운터 트리거 시스템 완성되면 제거
	GetWorldTimerManager().SetTimer(TempEncounterTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			TArray<APlayerState*> Players;
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				if (APlayerController* PC = It->Get())
				{
					if (APlayerState* PS = PC->PlayerState)
					{
						Players.Add(PS);
					}
				}
			}
			if (Players.Num() > 0)
			{
				SetParticipants(Players);
				GetWorldTimerManager().ClearTimer(TempEncounterTimer);
			}
		}),
		0.5f, /*bLoop=*/ true);
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
	if (bIsDead) return;

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
	if (bIsDead) return;

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

void AGYBossCharacterBase::GrantDefaultAbilities()
{
	Super::GrantDefaultAbilities();

	if (!AbilitySystemComponent) return;
	UBossDataAsset* BossData = GetBossData();
	if (!BossData) return;

	TSet<TSubclassOf<UGameplayAbility>> UniqueClasses;

	for (const FBossPatternEntry& Entry : BossData->NormalPatterns)
	{
		if (Entry.AbilityClass) UniqueClasses.Add(Entry.AbilityClass);
	}
	for (const FBossPhaseTrigger& Trigger : BossData->PhaseTriggers)
	{
		UGYBossPhaseAbility::CollectAbilities(Trigger.PhaseAbilityClass, UniqueClasses);
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : UniqueClasses)
	{
		if (!AbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass))
		{
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		}
	}
}

void AGYBossCharacterBase::OnDataAssetLoaded()
{
	Super::OnDataAssetLoaded();

	if (HasAuthority())
	{
		if (AGYBossAIController* AIC = Cast<AGYBossAIController>(GetController()))
		{
			if (UBossPatternSelectorComponent* Selector = AIC->GetPatternSelector())
			{
				if (UBossDataAsset* BossData = GetBossData())
				{
					Selector->InitializePatterns(BossData->NormalPatterns);
				}
			}
		}
	}

	RequestSummonablePreload();
}

void AGYBossCharacterBase::RequestSummonablePreload()
{
	UBossDataAsset* BossData = GetBossData();
	if (!BossData || BossData->SummonableEnemies.Num() == 0) return;

	TArray<FSoftObjectPath> Paths;
	Paths.Reserve(BossData->SummonableEnemies.Num() * 2);

	for (const FBossSummonEntry& Entry : BossData->SummonableEnemies)
	{
		if (!Entry.DataAsset.IsNull())  Paths.Add(Entry.DataAsset.ToSoftObjectPath());
		if (!Entry.ActorClass.IsNull()) Paths.Add(Entry.ActorClass.ToSoftObjectPath());
	}
	if (Paths.Num() == 0) return;

	FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();
	Streamable.RequestAsyncLoad(Paths,
		FStreamableDelegate::CreateWeakLambda(this, [this]()
		{
			OnSummonablesLoaded();
		}));
}

void AGYBossCharacterBase::OnSummonablesLoaded()
{
	UBossDataAsset* BossData = GetBossData();
	if (!BossData) return;

	SummonCache.Reset();
	for (const FBossSummonEntry& Entry : BossData->SummonableEnemies)
	{
		if (Entry.EnemyType == EEnemyType::None) continue;

		FBossCachedSummonable Cached;
		Cached.DataAsset  = Entry.DataAsset.Get();
		Cached.ActorClass = Entry.ActorClass.Get();
		Cached.HealthBleedRatio = Entry.HealthBleedRatio;

		if (Cached.DataAsset && Cached.ActorClass)
		{
			SummonCache.Add(Entry.EnemyType, Cached);
		}
	}
}

bool AGYBossCharacterBase::GetSummonable(EEnemyType Type, FBossCachedSummonable& Out) const
{
	if (const FBossCachedSummonable* Found = SummonCache.Find(Type))
	{
		Out = *Found;
		return true;
	}
	return false;
}

void AGYBossCharacterBase::RegisterMinion(AGYEnemyCharacterBase* Minion, float HealthBleedRatio)
{
	if (!Minion || !HasAuthority()) return;
	if (HealthBleedRatio <= 0.f) return;
	if (ActiveMinionRatios.Contains(Minion)) return;

	ActiveMinionRatios.Add(Minion, HealthBleedRatio);
	Minion->OnEnemyHit.AddDynamic(this, &AGYBossCharacterBase::HandleMinionDamaged);
	Minion->OnEnemyDead.AddDynamic(this, &AGYBossCharacterBase::HandleMinionDead);

	OnMinionCountChanged.Broadcast(ActiveMinionRatios.Num());
}

void AGYBossCharacterBase::HandleMinionDamaged(AGYEnemyCharacterBase* Minion, float DamageAmount)
{
	if (!Minion || !HasAuthority() || !VitalAttribute || !AbilitySystemComponent) return;

	const float* RatioPtr = ActiveMinionRatios.Find(Minion);
	if (!RatioPtr) return;

	const float Bleed = DamageAmount * (*RatioPtr);
	if (Bleed <= 0.f) return;

	AbilitySystemComponent->ApplyModToAttribute(
		UGYEnemyVitalAttributeSet::GetCurrentHealthAttribute(),
		EGameplayModOp::Additive,
		-Bleed);
}

void AGYBossCharacterBase::HandleMinionDead(AGYEnemyCharacterBase* Minion)
{
	if (!Minion) return;

	Minion->OnEnemyHit.RemoveDynamic(this, &AGYBossCharacterBase::HandleMinionDamaged);
	Minion->OnEnemyDead.RemoveDynamic(this, &AGYBossCharacterBase::HandleMinionDead);
	ActiveMinionRatios.Remove(Minion);

	OnMinionCountChanged.Broadcast(ActiveMinionRatios.Num());
}
