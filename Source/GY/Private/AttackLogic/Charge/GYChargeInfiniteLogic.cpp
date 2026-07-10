#include "AttackLogic/Charge/GYChargeInfiniteLogic.h"
#include "AttackLogic/Charge/GYChargeInfiniteFragment.h"
#include "AttackLogic/Charge/GYChargeInputLogic.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYChargeInfiniteLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	const UGYChargeInfiniteFragment* Fragment = Ability->GetFragment<UGYChargeInfiniteFragment>();
	if (!Fragment) return;

	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	FGameplayTagContainer OwnedTags;
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	if (!Fragment->IsActive(OwnedTags)) return;

	if (UGYChargeInputLogic* ChargeLogic = Ability->GetLogic<UGYChargeInputLogic>())
	{
		ChargeLogic->CancelMaxChargeTimer();
	}
}

TArray<FGameplayTag> UGYChargeInfiniteLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_ChargeInfinite };
}
