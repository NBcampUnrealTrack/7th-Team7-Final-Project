#include "AttackLogic/Combo/GYComboMontageFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYComboMontageFragment::UGYComboMontageFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ComboMontage;
}

const TArray<TObjectPtr<UAnimMontage>>* UGYComboMontageFragment::GetBestMatchingMontages(
	const FGameplayTagContainer& OwnedTags) const
{
	const TArray<TObjectPtr<UAnimMontage>>* DefaultResult = nullptr;

	for (const auto& Pair : MontageAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			if (!Pair.Value.Montages.IsEmpty())
				DefaultResult = &Pair.Value.Montages;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.Montages.IsEmpty() ? nullptr : &Pair.Value.Montages;
		}
	}

	return DefaultResult;
}
