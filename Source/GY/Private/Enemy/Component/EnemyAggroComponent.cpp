#include "Enemy/Component/EnemyAggroComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"
#include "Logging/GYLogManager.h"


UEnemyAggroComponent::UEnemyAggroComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
}

void UEnemyAggroComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	CachedAIController = Cast<AAIController>(GetOwner());
	if (!CachedAIController.IsValid()) return;

	BindToPerception();

	OnTargetChanged.AddDynamic(this, &UEnemyAggroComponent::HandleTargetChanged);

	GetWorld()->GetTimerManager().SetTimer(
		UpdateTimerHandle, this, &UEnemyAggroComponent::TickAggro, UpdateInterval, true);
}

void UEnemyAggroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Cur = CurrentTarget.Get())
	{
		if (auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Cur))
		{
			ASC->GetGameplayAttributeValueChangeDelegate(
				UGYVitalAttributeSet::GetCurrentHealthAttribute())
				.Remove(TargetHealthHandle);
		}
	}
	TargetHealthHandle.Reset();

	OnTargetChanged.RemoveDynamic(this, &UEnemyAggroComponent::HandleTargetChanged);

	UnbindFromPerception();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}

	for (const FAggroEntry& E : ThreatList)
	{
		RemoveTargetCombatTag(E.Actor.Get());
	}
	ThreatList.Reset();

	Super::EndPlay(EndPlayReason);
}


float UEnemyAggroComponent::GetThreatFor(AActor* Actor) const
{
	if (!Actor) return 0.f;
	const FAggroEntry* Entry = ThreatList.FindByPredicate(
		[Actor](const FAggroEntry& E)
		{
			return E.Actor.Get() == Actor;
		});
	return Entry ? Entry->Threat : 0.f;
}

TArray<FAggroEntry> UEnemyAggroComponent::GetTopThreats(int32 Count) const
{
	TArray<FAggroEntry> Copy = ThreatList;
	Copy.Sort([](const FAggroEntry& A, const FAggroEntry& B)
	{
		return A.Threat > B.Threat;
	});
	if (Count > 0 && Copy.Num() > Count) Copy.SetNum(Count);
	return Copy;
}

void UEnemyAggroComponent::AddThreat(AActor* Actor, float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	InternalAddThreat(Actor, Amount);
}

void UEnemyAggroComponent::ClearAllThreat()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (const FAggroEntry& E : ThreatList)
	{
		RemoveTargetCombatTag(E.Actor.Get());
	}

	AActor* Old = CurrentTarget.Get();
	ThreatList.Reset();
	SetCurrentTarget(nullptr);
}

void UEnemyAggroComponent::RemoveTargetCombatTag(AActor* Actor)
{
	if (!Actor) return;

	if (auto* TargetASC = Cast<UGYAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor)))
	{
		TargetASC->RemoveCombatTag();
	}
}

void UEnemyAggroComponent::ForceTarget(AActor* Actor, float ForcedThreatBonus)
{
	if (!Actor || !GetOwner() || !GetOwner()->HasAuthority()) return;

	InternalAddThreat(Actor, ForcedThreatBonus);
	SetCurrentTarget(Actor);
}

void UEnemyAggroComponent::BindToPerception()
{
	if (!CachedAIController.IsValid()) return;

	UAIPerceptionComponent* Perc = CachedAIController->GetPerceptionComponent();
	if (!Perc) return;

	CachedPerception = Perc;
	Perc->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &UEnemyAggroComponent::OnPerceptionUpdated);
	Perc->OnTargetPerceptionForgotten.AddUniqueDynamic(this, &UEnemyAggroComponent::OnPerceptionForgotten);
}

void UEnemyAggroComponent::UnbindFromPerception()
{
	if (CachedPerception.IsValid())
	{
		CachedPerception->OnTargetPerceptionUpdated.RemoveDynamic(this,
			&UEnemyAggroComponent::OnPerceptionUpdated);
		CachedPerception->OnTargetPerceptionForgotten.RemoveDynamic(this,
			&UEnemyAggroComponent::OnPerceptionForgotten);
	}
	CachedPerception.Reset();
}

void UEnemyAggroComponent::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// UE_LOG(LogTemp, Warning,
	// 	TEXT("[EnemyAggro] PerceptionUpdated Actor=%s Type=%s Sensed=%d Strength=%.1f"),
	// 	*GetNameSafe(Actor),
	// 	*Stimulus.Type.Name.ToString(),
	// 	Stimulus.WasSuccessfullySensed() ? 1 : 0,
	// 	Stimulus.Strength);
	if (!Actor || !GetOwner() || !GetOwner()->HasAuthority()) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (TargetASC && TargetASC->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy))
	{
		return;
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
			InternalAddThreat(Actor, Weights.NoiseOnHeard);
		}
	}
}

void UEnemyAggroComponent::OnPerceptionForgotten(AActor* Actor)
{
}

void UEnemyAggroComponent::HandleTargetChanged(AActor* OldTarget, AActor* NewTarget)
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

void UEnemyAggroComponent::SetCurrentTarget(AActor* NewTarget)
{
	AActor* Old = CurrentTarget.Get();
	if (Old == NewTarget) return;

	if (Old)
	{
		if (auto* OldASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Old))
		{
			OldASC->GetGameplayAttributeValueChangeDelegate(
				UGYVitalAttributeSet::GetCurrentHealthAttribute())
				.Remove(TargetHealthHandle);
		}
	}
	TargetHealthHandle.Reset();

	if (NewTarget)
	{
		if (auto* NewASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewTarget))
		{
			TargetHealthHandle = NewASC->GetGameplayAttributeValueChangeDelegate(
				UGYVitalAttributeSet::GetCurrentHealthAttribute())
				.AddUObject(this, &UEnemyAggroComponent::OnTargetHealthChanged);
		}
	}

	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(Old, NewTarget);
}

void UEnemyAggroComponent::OnTargetHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue > 0.f) return;

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	AActor* Target = CurrentTarget.Get();
	if (!Target) return;

	for (int32 i = ThreatList.Num() - 1; i >= 0; --i)
	{
		if (ThreatList[i].Actor.Get() == Target)
		{
			RemoveTargetCombatTag(Target);
			ThreatList.RemoveAt(i);
			break;
		}
	}

	SetCurrentTarget(nullptr);
}


void UEnemyAggroComponent::TickAggro()
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

		if (!IsTargetTargetable(E.Actor.Get()))
		{
			RemoveTargetCombatTag(E.Actor.Get());
			ThreatList.RemoveAt(i);
			continue;
		}

		E.Threat = FMath::Max(0.f, E.Threat - DecayAmount);

		const bool bExpired = (Now - E.LastUpdateTime) > Weights.ForgetTime;
		if (bExpired || E.Threat <= KINDA_SMALL_NUMBER)
		{
			RemoveTargetCombatTag(E.Actor.Get());
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
			SetCurrentTarget(TopActor);
		}
	}
	else if (TopActor == nullptr && OldTarget != nullptr)
	{
		SetCurrentTarget(nullptr);
	}
}

bool UEnemyAggroComponent::IsTargetTargetable(AActor* Actor)
{
	if (!Actor) return false;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (!ASC) return true;

	return !ASC->HasMatchingGameplayTag(GYStateTags::State_Life_Dead)
		&& !ASC->HasMatchingGameplayTag(GYStateTags::State_Life_Downed);
}

void UEnemyAggroComponent::InternalAddThreat(AActor* Actor, float Amount)
{
	if (!Actor || Amount <= 0.f) return;
	if (!IsTargetTargetable(Actor)) return;

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

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
		UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC);
		if (GYASC)
		{
			GYASC->ApplyCombatTag();
		}
	}
}
