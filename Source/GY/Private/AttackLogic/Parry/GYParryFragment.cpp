#include "AttackLogic/Parry/GYParryFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYParryFragment::UGYParryFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_Parry;
}

const FGYParryData* UGYParryFragment::GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const
{
	const FGYParryData* DefaultResult = nullptr;

	for (const auto& Pair : ParryDataSets)
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
