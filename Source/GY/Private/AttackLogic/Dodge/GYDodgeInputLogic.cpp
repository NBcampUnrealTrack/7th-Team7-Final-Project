#include "AttackLogic/Dodge/GYDodgeInputLogic.h"
#include "AttackLogic/Dodge/GYDodgeFragment.h"
#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "AttackLogic/Shared/GYAttributeCostHelpers.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"

using GYAttributeCostHelpers::ApplyCost;

void UGYDodgeInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;

	FGameplayTagContainer OwnedTags;
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	const FGYDodgeData* DodgeData = nullptr;
	const FGYDodgeMontageSet* MontageSet = nullptr;

	if (const UGYDodgeFragment* DF = Ability->GetFragment<UGYDodgeFragment>())
	{
		DodgeData = DF->GetBestMatchingData(OwnedTags);
	}

	if (const UGYDodgeMontageFragment* MF = Ability->GetFragment<UGYDodgeMontageFragment>())
	{
		MontageSet = MF->GetBestMatchingSet(OwnedTags);
	}

	if (!DodgeData || !MontageSet)
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get())
			{
				A->RequestEnd(true);
			}
		});
		return;
	}

	const UGYDodgeFragment* DodgeFragment = Ability->GetFragment<UGYDodgeFragment>();
	CachedDodgeAppliedTag = DodgeFragment ? DodgeFragment->DodgeAppliedTag : FGameplayTag();

	ApplyCost(ASC, DodgeData->StaminaCost);

	const float Duration = Ability->PlayMontageForLogic(MontageSet->DodgeMontage, 1.f);

	if (ASC && CachedDodgeAppliedTag.IsValid() && DodgeData->InvincibilityDuration > 0.f)
	{
		ASC->AddLooseGameplayTag(CachedDodgeAppliedTag);

		TWeakObjectPtr<UGYDodgeInputLogic> WeakThis(this);
		Ability->GetWorld()->GetTimerManager().SetTimer(
			IFrameTimer,
			[WeakThis]()
			{
				if (UGYDodgeInputLogic* Self = WeakThis.Get())
				{
					Self->RemoveDodgeTag();
				}
			},
			FMath::Max(DodgeData->InvincibilityDuration, KINDA_SMALL_NUMBER),
			false
		);
	}

	TWeakObjectPtr<UGYDodgeInputLogic> WeakThis(this);
	Ability->GetWorld()->GetTimerManager().SetTimer(
		EndTimer,
		[WeakThis]()
		{
			if (UGYDodgeInputLogic* Self = WeakThis.Get())
			{
				if (Self->CachedAbility.IsValid())
				{
					Self->CachedAbility->RequestEnd(false);
				}
			}
		},
		FMath::Max(Duration, 0.1f),
		false
	);
}

void UGYDodgeInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	RemoveDodgeTag();
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYDodgeInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Dodge,
		GYGameplayTags::Ability_Fragment_DodgeMontage
	};
}

void UGYDodgeInputLogic::RemoveDodgeTag()
{
	if (!CachedAbility.IsValid() || !CachedDodgeAppliedTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedDodgeAppliedTag))
	{
		ASC->RemoveLooseGameplayTag(CachedDodgeAppliedTag);
	}
}
