#include "AttackLogic/Charge/GYChargeMontageFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYChargeMontageFragment::UGYChargeMontageFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ChargeMontage;
}

const FGYChargeMontageSet* UGYChargeMontageFragment::GetBestMatchingSet(
	const FGameplayTagContainer& OwnedTags) const
{
	const FGYChargeMontageSet* DefaultResult = nullptr;

	for (const auto& Pair : MontageAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			if (Pair.Value.ChargeMontage)
				DefaultResult = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.ChargeMontage ? &Pair.Value : nullptr;
		}
	}

	return DefaultResult;
}
