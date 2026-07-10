#include "AttackLogic/Parry/GYParryCounterFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYParryCounterFragment::UGYParryCounterFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ParryCounter;
}

const FGYParryCounterSet* UGYParryCounterFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYParryCounterSet* DefaultResult = nullptr;

	for (const FGYParryCounterEntry& Entry : AttackSets)
	{
		if (Entry.RequiredTags.IsEmpty())
		{
			DefaultResult = &Entry.Set;
			continue;
		}
		if (OwnedTags.HasAll(Entry.RequiredTags))
		{
			return &Entry.Set;
		}
	}

	return DefaultResult;
}
