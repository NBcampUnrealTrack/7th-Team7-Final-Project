#include "AttackLogic/Block/GYBlockFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYBlockFragment::UGYBlockFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_Block;
}

const FGYBlockData* UGYBlockFragment::GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const
{
	const FGYBlockData* DefaultResult = nullptr;

	for (const auto& Pair : BlockDataSets)
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
