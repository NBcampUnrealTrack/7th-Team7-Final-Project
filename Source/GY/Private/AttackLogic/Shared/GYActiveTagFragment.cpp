#include "AttackLogic/Shared/GYActiveTagFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYActiveTagFragment::UGYActiveTagFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ActiveTag;
}

const FGameplayTagContainer* UGYActiveTagFragment::GetBestMatchingTags(const FGameplayTagContainer& OwnedTags) const
{
	const FGameplayTagContainer* DefaultResult = nullptr;

	for (const auto& Pair : TagSets)
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
