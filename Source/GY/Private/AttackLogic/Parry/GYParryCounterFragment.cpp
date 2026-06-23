#include "AttackLogic/Parry/GYParryCounterFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYParryCounterFragment::UGYParryCounterFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ParryCounter;
}

const FGYParryCounterSet* UGYParryCounterFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYParryCounterSet* DefaultResult = nullptr;

	for (const auto& Pair : AttackSets)
	{
		if (!Pair.Key.IsValid())
		{
			DefaultResult = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return &Pair.Value;
		}
	}

	return DefaultResult;
}
