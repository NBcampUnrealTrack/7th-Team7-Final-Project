#include "AttackLogic/Block/GYBlockInputLogic.h"
#include "AttackLogic/Block/GYBlockFragment.h"
#include "AttackLogic/Block/GYBlockMontageFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
void UGYBlockInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedMontageSet = nullptr;
	bEnding = false;

	FGameplayTagContainer OwnedTags;
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC) ASC->GetOwnedGameplayTags(OwnedTags);

	FGameplayTagContainer FallbackTags;
	if (Ability->DefaultWeaponTypeTag.IsValid())
		FallbackTags.AddTag(Ability->DefaultWeaponTypeTag);

	const UGYBlockFragment* Fragment = Ability->GetFragment<UGYBlockFragment>();
	const UGYBlockMontageFragment* MontageFragment = Ability->GetFragment<UGYBlockMontageFragment>();

	if (!Fragment)
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get()) A->RequestEnd(true);
		});
		return;
	}

	const FGYBlockData* BlockData = Fragment->GetBestMatchingData(OwnedTags);
	if (!BlockData && !FallbackTags.IsEmpty())
		BlockData = Fragment->GetBestMatchingData(FallbackTags);

	if (!BlockData)
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get()) A->RequestEnd(true);
		});
		return;
	}

	if (MontageFragment)
	{
		CachedMontageSet = MontageFragment->GetBestMatchingSet(OwnedTags);
		if (!CachedMontageSet && !FallbackTags.IsEmpty())
			CachedMontageSet = MontageFragment->GetBestMatchingSet(FallbackTags);
	}

	CachedBlockAppliedTag = Fragment->BlockAppliedTag;
	CachedDrainAttribute = BlockData->StaminaDrainPerSecond.Attribute;
	CachedDrainPerSecond = BlockData->StaminaDrainPerSecond.Amount;
	CachedHitCostAttribute = BlockData->BlockHitCostAttribute;
	CachedHitCostMultiplier = BlockData->BlockHitCostMultiplier;

	if (ASC && CachedBlockAppliedTag.IsValid())
		ASC->AddLooseGameplayTag(CachedBlockAppliedTag);

	if (CachedMontageSet && CachedMontageSet->HoldMontage)
	{
		Ability->PlayMontageForLogic(CachedMontageSet->HoldMontage, 1.f);
	}

	if (CachedDrainPerSecond > 0.f && CachedDrainAttribute.IsValid())
	{
		TWeakObjectPtr<UGYBlockInputLogic> WeakThis(this);
		Ability->GetWorld()->GetTimerManager().SetTimer(
			DrainTimer,
			[WeakThis]() { if (UGYBlockInputLogic* Self = WeakThis.Get()) Self->DrainTick(); },
			DrainInterval,
			true
		);
	}
}

void UGYBlockInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(DrainTimer);
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(EndMontageTimer);
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(BlockBreakTimer);
	}
	RemoveBlockTag();
	CachedAbility.Reset();
	CachedMontageSet = nullptr;
}

TArray<FGameplayTag> UGYBlockInputLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Block_Hit };
}

void UGYBlockInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (EventTag != GYGameplayTags::Event_Block_Hit || bEnding || !CachedAbility.IsValid()) return;
	if (!CachedHitCostAttribute.IsValid() || CachedHitCostMultiplier <= 0.f) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const float Cost = Payload.EventMagnitude * CachedHitCostMultiplier;
	const float Current = ASC->GetNumericAttributeBase(CachedHitCostAttribute);
	const float NewValue = Current - Cost;
	ASC->SetNumericAttributeBase(CachedHitCostAttribute, NewValue);

	if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(ASC))
		GYASC->NotifyAttributeChanged(CachedHitCostAttribute);

	if (NewValue <= 0.f)
		PlayBlockBreak();
}

void UGYBlockInputLogic::OnInputReleased()
{
	PlayBlockEnd();
}

TArray<FGameplayTag> UGYBlockInputLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_Block };
}

void UGYBlockInputLogic::DrainTick()
{
	UGYPlayerGameplayAbility* Ability = CachedAbility.Get();
	if (!Ability) return;

	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const float Current = ASC->GetNumericAttributeBase(CachedDrainAttribute);
	if (Current <= 0.f)
	{
		PlayBlockEnd();
		return;
	}

	const float Drain = CachedDrainPerSecond * DrainInterval;
	ASC->SetNumericAttributeBase(CachedDrainAttribute, FMath::Max(0.f, Current - Drain));

	if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(ASC))
		GYASC->NotifyAttributeChanged(CachedDrainAttribute);

	if (Current - Drain <= 0.f)
		PlayBlockEnd();
}

void UGYBlockInputLogic::PlayBlockEnd()
{
	if (bEnding || !CachedAbility.IsValid()) return;
	bEnding = true;

	CachedAbility->GetWorld()->GetTimerManager().ClearTimer(DrainTimer);
	RemoveBlockTag();

	if (CachedMontageSet && CachedMontageSet->EndMontage)
	{
		const float Duration = CachedAbility->PlayMontageForLogic(CachedMontageSet->EndMontage, 1.f);
		TWeakObjectPtr<UGYBlockInputLogic> WeakThis(this);
		CachedAbility->GetWorld()->GetTimerManager().SetTimer(
			EndMontageTimer,
			[WeakThis]()
			{
				if (UGYBlockInputLogic* Self = WeakThis.Get())
					if (Self->CachedAbility.IsValid())
						Self->CachedAbility->RequestEnd(false);
			},
			FMath::Max(Duration, 0.1f),
			false
		);
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYBlockInputLogic::PlayBlockBreak()
{
	if (bEnding || !CachedAbility.IsValid()) return;
	bEnding = true;

	CachedAbility->GetWorld()->GetTimerManager().ClearTimer(DrainTimer);
	RemoveBlockTag();

	if (CachedMontageSet && CachedMontageSet->BlockBreakMontage)
	{
		const float Duration = CachedAbility->PlayMontageForLogic(CachedMontageSet->BlockBreakMontage, 1.f);
		TWeakObjectPtr<UGYBlockInputLogic> WeakThis(this);
		CachedAbility->GetWorld()->GetTimerManager().SetTimer(
			BlockBreakTimer,
			[WeakThis]()
			{
				if (UGYBlockInputLogic* Self = WeakThis.Get())
					if (Self->CachedAbility.IsValid())
						Self->CachedAbility->RequestEnd(false);
			},
			FMath::Max(Duration, 0.1f),
			false
		);
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYBlockInputLogic::RemoveBlockTag()
{
	if (!CachedAbility.IsValid() || !CachedBlockAppliedTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedBlockAppliedTag))
		ASC->RemoveLooseGameplayTag(CachedBlockAppliedTag);
}
