#include "AttackLogic/Combo/GYComboFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYComboFragment::UGYComboFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_Attack;
}

const TArray<float>* UGYComboFragment::GetBestMatchingMultipliers(const FGameplayTagContainer& OwnedTags) const
{
	const TArray<float>* DefaultResult = nullptr;

	for (const auto& Pair : DamageMultipliers)
	{
		if (!Pair.Key.IsValid())
		{
			DefaultResult = &Pair.Value.Multipliers;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return &Pair.Value.Multipliers;
		}
	}

	return DefaultResult;
}
