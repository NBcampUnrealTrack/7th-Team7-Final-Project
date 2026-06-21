#include "Enemy/Component/BossAggroComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"
#include "Logging/GYLogManager.h"


UBossAggroComponent::UBossAggroComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
}

void UBossAggroComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	CachedAIController = Cast<AAIController>(GetOwner());
	if (!CachedAIController.IsValid()) return;

	BindToPerception();

	OnTargetChanged.AddDynamic(this, &UBossAggroComponent::HandleTargetChanged);

	GetWorld()->GetTimerManager().SetTimer(
		UpdateTimerHandle, this, &UBossAggroComponent::TickAggro, UpdateInterval, true);
}

void UBossAggroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnTargetChanged.RemoveDynamic(this, &UBossAggroComponent::HandleTargetChanged);

	UnbindFromPerception();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}


float UBossAggroComponent::GetThreatFor(AActor* Actor) const
{
	if (!Actor) return 0.f;
	const FAggroEntry* Entry = ThreatList.FindByPredicate(
		[Actor](const FAggroEntry& E)
		{
			return E.Actor.Get() == Actor;
		});
	return Entry ? Entry->Threat : 0.f;
}

TArray<FAggroEntry> UBossAggroComponent::GetTopThreats(int32 Count) const
{
	TArray<FAggroEntry> Copy = ThreatList;
	Copy.Sort([](const FAggroEntry& A, const FAggroEntry& B)
	{
		return A.Threat > B.Threat;
	});
	if (Count > 0 && Copy.Num() > Count) Copy.SetNum(Count);
	return Copy;
}

void UBossAggroComponent::AddThreat(AActor* Actor, float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	InternalAddThreat(Actor, Amount);
}

void UBossAggroComponent::ClearAllThreat()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	AActor* Old = CurrentTarget.Get();
	ThreatList.Reset();
	CurrentTarget.Reset();

	if (Old != nullptr)
	{
		OnTargetChanged.Broadcast(Old, nullptr);
	}
}

void UBossAggroComponent::ForceTarget(AActor* Actor, float ForcedThreatBonus)
{
	if (!Actor || !GetOwner() || !GetOwner()->HasAuthority()) return;

	InternalAddThreat(Actor, ForcedThreatBonus);

	AActor* Old = CurrentTarget.Get();
	CurrentTarget = Actor;
	if (Old != Actor)
	{
		OnTargetChanged.Broadcast(Old, Actor);
	}
}

void UBossAggroComponent::BindToPerception()
{
	if (!CachedAIController.IsValid()) return;

	UAIPerceptionComponent* Perc = CachedAIController->GetPerceptionComponent();
	if (!Perc) return;

	CachedPerception = Perc;
	Perc->OnTargetPerceptionUpdated.AddDynamic(this, &UBossAggroComponent::OnPerceptionUpdated);
	Perc->OnTargetPerceptionForgotten.AddDynamic(this, &UBossAggroComponent::OnPerceptionForgotten);
}

void UBossAggroComponent::UnbindFromPerception()
{
	if (CachedPerception.IsValid())
	{
		CachedPerception->OnTargetPerceptionUpdated.RemoveDynamic(this,
			&UBossAggroComponent::OnPerceptionUpdated);
		CachedPerception->OnTargetPerceptionForgotten.RemoveDynamic(this,
			&UBossAggroComponent::OnPerceptionForgotten);
	}
	CachedPerception.Reset();
}

void UBossAggroComponent::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !GetOwner() || !GetOwner()->HasAuthority()) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (TargetASC && TargetASC->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC);
		if (GYASC)
		{
			GYASC->ApplyCombatTag();
		}
	}

	const FAISenseID SightId = UAISense::GetSenseID<UAISense_Sight>();
	const FAISenseID DamageId = UAISense::GetSenseID<UAISense_Damage>();
	const FAISenseID HearId = UAISense::GetSenseID<UAISense_Hearing>();

	if (Stimulus.Type == SightId)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			InternalAddThreat(Actor, Weights.SightOnSpotted);
		}
	}
	else if (Stimulus.Type == DamageId)
	{
		const float Dmg = FMath::Max(0.f, Stimulus.Strength);
		InternalAddThreat(Actor, Dmg * Weights.DamagePerHitMultiplier);
	}
	else if (Stimulus.Type == HearId)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (FAggroEntry* Entry = ThreatList.FindByPredicate(
				[Actor](const FAggroEntry& E) { return E.Actor.Get() == Actor; }))
			{
				Entry->Threat += Weights.NoiseOnHeard;
				Entry->LastUpdateTime = GetWorld()->GetTimeSeconds();
			}
		}
	}
}

void UBossAggroComponent::OnPerceptionForgotten(AActor* Actor)
{
}

void UBossAggroComponent::HandleTargetChanged(AActor* OldTarget, AActor* NewTarget)
{
	UE_LOG(LogTemp, Warning, TEXT("[BossAggro] HandleTargetChanged Old=%s New=%s"),
		*GetNameSafe(OldTarget), *GetNameSafe(NewTarget));
	if (!CachedAIController.IsValid()) return;

	APawn* SelfPawn = CachedAIController->GetPawn();
	if (!SelfPawn) return;

	auto* SelfASC = Cast<UGYEnemyAbilitySystemComponent>(
		SelfPawn->FindComponentByClass<UGYEnemyAbilitySystemComponent>());
	if (!SelfASC) return;

	if (NewTarget)
	{
		SelfASC->ApplyCombatTag();
	}
	else
	{
		SelfASC->RemoveCombatTag();
	}
}

void UBossAggroComponent::TickAggro()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	const float Now = GetWorld()->GetTimeSeconds();
	const float DeltaTime = UpdateInterval;
	const float DecayAmount = Weights.ThreatDecayPerSecond * DeltaTime;

	if (CachedPerception.IsValid())
	{
		TArray<AActor*> SightedActors;
		CachedPerception->GetCurrentlyPerceivedActors(
			UAISense_Sight::StaticClass(), SightedActors);

		const float SightTickGain = DecayAmount + KINDA_SMALL_NUMBER;
		for (AActor* A : SightedActors)
		{
			UAbilitySystemComponent* TargetASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A);
			if (TargetASC && TargetASC->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy))
				continue;

			InternalAddThreat(A, SightTickGain);
		}
	}

	for (int32 i = ThreatList.Num() - 1; i >= 0; --i)
	{
		FAggroEntry& E = ThreatList[i];

		if (!E.Actor.IsValid())
		{
			ThreatList.RemoveAt(i);
			continue;
		}

		E.Threat = FMath::Max(0.f, E.Threat - DecayAmount);

		const bool bExpired = (Now - E.LastUpdateTime) > Weights.ForgetTime;
		if (bExpired || E.Threat <= KINDA_SMALL_NUMBER)
		{
			ThreatList.RemoveAt(i);
		}
	}

	AActor* TopActor = nullptr;
	float TopThreat = -1.f;
	for (const FAggroEntry& E : ThreatList)
	{
		if (E.Threat > TopThreat)
		{
			TopThreat = E.Threat;
			TopActor = E.Actor.Get();
		}
	}

	AActor* OldTarget = CurrentTarget.Get();
	if (TopActor != OldTarget && TopActor != nullptr)
	{
		const float CurrentThreat = GetThreatFor(OldTarget);
		if (OldTarget == nullptr || TopThreat > CurrentThreat + Weights.SwitchHysteresis)
		{
			CurrentTarget = TopActor;
			OnTargetChanged.Broadcast(OldTarget, TopActor);
		}
	}
	else if (TopActor == nullptr && OldTarget != nullptr)
	{
		CurrentTarget.Reset();
		OnTargetChanged.Broadcast(OldTarget, nullptr);
	}
}

void UBossAggroComponent::InternalAddThreat(AActor* Actor, float Amount)
{
	if (!Actor || Amount <= 0.f) return;

	const float Now = GetWorld()->GetTimeSeconds();

	FAggroEntry* Entry = ThreatList.FindByPredicate(
		[Actor](const FAggroEntry& E)
	{
		return E.Actor.Get() == Actor;
	});

	if (Entry)
	{
		Entry->Threat += Amount;
		Entry->LastUpdateTime = Now;
	}
	else
	{
		FAggroEntry NewEntry;
		NewEntry.Actor = Actor;
		NewEntry.Threat = Amount;
		NewEntry.LastUpdateTime = Now;
		ThreatList.Add(NewEntry);
	}
}
