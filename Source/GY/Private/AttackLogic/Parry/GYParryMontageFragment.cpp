#include "AttackLogic/Parry/GYParryMontageFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYParryMontageFragment::UGYParryMontageFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ParryMontage;
}

const FGYParryMontageSet* UGYParryMontageFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYParryMontageSet* DefaultResult = nullptr;

	for (const auto& Pair : MontageAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			if (Pair.Value.ParryMontage)
				DefaultResult = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.ParryMontage ? &Pair.Value : nullptr;
		}
	}

	return DefaultResult;
}
