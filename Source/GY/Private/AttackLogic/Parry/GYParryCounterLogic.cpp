#include "AttackLogic/Parry/GYParryCounterLogic.h"
#include "AttackLogic/Parry/GYParryInputLogic.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"

UGYParryCounterLogic::UGYParryCounterLogic()
{
	TriggerEventTag = GYGameplayTags::Event_Input_Attack;
}

void UGYParryCounterLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedCounterSet = nullptr;
	bWindowOpen = false;

	if (const UGYParryCounterFragment* Fragment = Ability->GetFragment<UGYParryCounterFragment>())
	{
		CachedWindowTimeout = Fragment->WindowTimeout;
	}
}

void UGYParryCounterLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CloseWindow();
	CachedCounterSet = nullptr;
	if (CounterMontageTask) { CounterMontageTask->EndTask(); CounterMontageTask = nullptr; }
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYParryCounterLogic::GetSubscribedEventTags() const
{
	TArray<FGameplayTag> Tags = { GYGameplayTags::Event_Parry_Hit, GYGameplayTags::Event_Anim_Attack_DoTrace };
	if (TriggerEventTag.IsValid()) Tags.Add(TriggerEventTag);
	return Tags;
}

void UGYParryCounterLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (EventTag == GYGameplayTags::Event_Parry_Hit)
	{
		OpenWindow();
		return;
	}

	if (EventTag == TriggerEventTag && bWindowOpen)
	{
		ExecuteCounterAttack();
		return;
	}

	if (EventTag == GYGameplayTags::Event_Anim_Attack_DoTrace)
	{
		ApplyHit(Payload);
	}
}

TArray<FGameplayTag> UGYParryCounterLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_ParryCounter };
}

const FGYCollisionShapeData* UGYParryCounterLogic::GetCurrentCollisionData() const
{
	return CachedCounterSet ? &CachedCounterSet->CollisionData : nullptr;
}

void UGYParryCounterLogic::OpenWindow()
{
	if (!CachedAbility.IsValid()) return;

	bWindowOpen = true;

	if (WindowTimeoutTask) { WindowTimeoutTask->EndTask(); WindowTimeoutTask = nullptr; }
	WindowTimeoutTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), CachedWindowTimeout);
	WindowTimeoutTask->OnFinish.AddDynamic(this, &UGYParryCounterLogic::OnWindowTimedOut);
	WindowTimeoutTask->ReadyForActivation();
}

void UGYParryCounterLogic::CloseWindow()
{
	bWindowOpen = false;
	if (WindowTimeoutTask) { WindowTimeoutTask->EndTask(); WindowTimeoutTask = nullptr; }
}

void UGYParryCounterLogic::OnWindowTimedOut()
{
	WindowTimeoutTask = nullptr;
	CloseWindow();
}

void UGYParryCounterLogic::ExecuteCounterAttack()
{
	if (!CachedAbility.IsValid()) return;

	CloseWindow();

	if (UGYParryInputLogic* ParryLogic = CachedAbility->GetLogic<UGYParryInputLogic>())
	{
		ParryLogic->CancelCounterMontageTask();
	}

	const UGYParryCounterFragment* Fragment = CachedAbility->GetFragment<UGYParryCounterFragment>();
	if (!Fragment) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const FGYParryCounterSet* Set = Fragment->GetBestMatchingSet(OwnedTags);
	if (!Set || !Set->Montage) return;

	CachedCounterSet = Set;
	CachedAbility->SetCurrentHitImpact(Set->HitImpact);

	const float Duration = CachedAbility->PlayMontageForLogic(Set->Montage, 1.f);

	CounterMontageTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(Duration, 0.1f));
	CounterMontageTask->OnFinish.AddDynamic(this, &UGYParryCounterLogic::OnCounterMontageFinished);
	CounterMontageTask->ReadyForActivation();
}

void UGYParryCounterLogic::ApplyHit(const FGameplayEventData& Payload)
{
	if (!CachedCounterSet || !CachedAbility.IsValid()) return;
	if (!CachedAbility->GetCurrentActorInfo()->IsNetAuthority()) return;

	const AActor* TargetActor = Payload.Target.Get();
	if (!TargetActor) return;

	UAbilitySystemComponent* InstigatorASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	const IAbilitySystemInterface* ASCInterface = Cast<const IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	if (!InstigatorASC || !TargetASC) return;

	const FGYHitImpact& Impact = CachedAbility->GetCurrentHitImpact();

	FGYHitContext HitContext;
	HitContext.SourceASC = InstigatorASC;
	HitContext.TargetASC = TargetASC;
	HitContext.MotionMultiplier = Impact.DamageMultiplier;
	HitContext.StaggerAmount = Impact.StaggerAmount;
	HitContext.StunAmount = Impact.StunAmount;

	UGYCombatStatics::ApplyHitImpact(HitContext);
}

void UGYParryCounterLogic::OnCounterMontageFinished()
{
	CounterMontageTask = nullptr;
	CachedCounterSet = nullptr;
	if (CachedAbility.IsValid())
	{
		CachedAbility->RequestEnd(false);
	}
}
