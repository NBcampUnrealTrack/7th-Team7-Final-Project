#include "AttackLogic/Block/GYBlockAttackLogic.h"
#include "AttackLogic/Block/GYBlockMontageFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"

UGYBlockAttackLogic::UGYBlockAttackLogic()
{
	TriggerEventTag = GYGameplayTags::Event_Input_Attack;
}

void UGYBlockAttackLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedAttackSet = nullptr;
	bWindowOpen = false;

	if (const UGYBlockAttackFragment* Fragment = Ability->GetFragment<UGYBlockAttackFragment>())
	{
		CachedWindowTimeout = Fragment->WindowTimeout;
	}
}

void UGYBlockAttackLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CloseWindow();
	CachedAttackSet = nullptr;
	if (AttackMontageTask) { AttackMontageTask->EndTask(); AttackMontageTask = nullptr; }
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYBlockAttackLogic::GetSubscribedEventTags() const
{
	TArray<FGameplayTag> Tags = { GYGameplayTags::Event_Block_Hit, GYGameplayTags::Event_Anim_Attack_DoTrace };
	if (TriggerEventTag.IsValid()) Tags.Add(TriggerEventTag);
	return Tags;
}

void UGYBlockAttackLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (EventTag == GYGameplayTags::Event_Block_Hit)
	{
		OpenWindow();
		return;
	}

	if (EventTag == TriggerEventTag && bWindowOpen)
	{
		ExecuteBlockAttack();
		return;
	}

	if (EventTag == GYGameplayTags::Event_Anim_Attack_DoTrace)
	{
		ApplyHit(Payload);
	}
}

TArray<FGameplayTag> UGYBlockAttackLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_BlockAttack };
}

const FGYCollisionShapeData* UGYBlockAttackLogic::GetCurrentCollisionData() const
{
	return CachedAttackSet ? &CachedAttackSet->CollisionData : nullptr;
}

void UGYBlockAttackLogic::OpenWindow()
{
	if (!CachedAbility.IsValid()) return;

	bWindowOpen = true;

	CachedAbility->GetWorld()->GetTimerManager().ClearTimer(WindowTimer);
	TWeakObjectPtr<UGYBlockAttackLogic> WeakThis(this);
	CachedAbility->GetWorld()->GetTimerManager().SetTimer(
		WindowTimer,
		[WeakThis]() { if (UGYBlockAttackLogic* Self = WeakThis.Get()) Self->CloseWindow(); },
		CachedWindowTimeout,
		false
	);
}

void UGYBlockAttackLogic::CloseWindow()
{
	bWindowOpen = false;
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(WindowTimer);
	}
}

void UGYBlockAttackLogic::ExecuteBlockAttack()
{
	if (!CachedAbility.IsValid()) return;

	CloseWindow();

	const UGYBlockAttackFragment* Fragment = CachedAbility->GetFragment<UGYBlockAttackFragment>();
	if (!Fragment) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const FGYBlockAttackSet* Set = Fragment->GetBestMatchingSet(OwnedTags);
	if (!Set || !Set->Montage) return;

	CachedAttackSet = Set;
	CachedAbility->SetCurrentHitImpact(Set->HitImpact);

	const float Duration = CachedAbility->PlayMontageForLogic(Set->Montage, 1.f);

	AttackMontageTask = UAbilityTask_WaitDelay::WaitDelay(CachedAbility.Get(), FMath::Max(Duration, 0.1f));
	AttackMontageTask->OnFinish.AddDynamic(this, &UGYBlockAttackLogic::OnAttackMontageFinished);
	AttackMontageTask->ReadyForActivation();
}

void UGYBlockAttackLogic::ApplyHit(const FGameplayEventData& Payload)
{
	if (!CachedAttackSet || !CachedAbility.IsValid()) return;
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

void UGYBlockAttackLogic::OnAttackMontageFinished()
{
	AttackMontageTask = nullptr;
	CachedAttackSet = nullptr;
	if (!CachedAbility.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const UGYBlockMontageFragment* MontageFragment = CachedAbility->GetFragment<UGYBlockMontageFragment>();
	if (!MontageFragment) return;

	const FGYBlockMontageSet* MontageSet = MontageFragment->GetBestMatchingSet(OwnedTags);
	if (MontageSet && MontageSet->HoldMontage)
	{
		CachedAbility->PlayMontageForLogic(MontageSet->HoldMontage, 1.f);
	}
}
