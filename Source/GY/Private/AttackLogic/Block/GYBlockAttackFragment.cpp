#include "AttackLogic/Block/GYBlockAttackFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYBlockAttackFragment::UGYBlockAttackFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_BlockAttack;
}

const FGYBlockAttackSet* UGYBlockAttackFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYBlockAttackSet* DefaultResult = nullptr;

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
