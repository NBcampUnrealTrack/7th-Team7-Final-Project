#include "AttackLogic/Block/GYBlockAttackFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYBlockAttackFragment::UGYBlockAttackFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_BlockAttack;
}

const FGYBlockAttackSet* UGYBlockAttackFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYBlockAttackSet* DefaultResult = nullptr;

	for (const FGYBlockAttackEntry& Entry : AttackSets)
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
