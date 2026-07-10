#include "AttackLogic/Shared/GYActiveTagFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYActiveTagFragment::UGYActiveTagFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ActiveTag;
}

const FGameplayTagContainer* UGYActiveTagFragment::GetBestMatchingTags(const FGameplayTagContainer& OwnedTags) const
{
	const FGameplayTagContainer* DefaultResult = nullptr;

	for (const FGYActiveTagEntry& Entry : TagSets)
	{
		if (Entry.RequiredTags.IsEmpty())
		{
			DefaultResult = &Entry.TagsToApply;
			continue;
		}
		if (OwnedTags.HasAll(Entry.RequiredTags))
		{
			return &Entry.TagsToApply;
		}
	}

	return DefaultResult;
}
