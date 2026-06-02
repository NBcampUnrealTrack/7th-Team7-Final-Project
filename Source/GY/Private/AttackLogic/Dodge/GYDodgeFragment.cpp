#include "AttackLogic/Dodge/GYDodgeFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYDodgeFragment::UGYDodgeFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_Dodge;
}

const FGYDodgeData* UGYDodgeFragment::GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const
{
	const FGYDodgeData* DefaultResult = nullptr;

	for (const auto& Pair : DodgeDataSets)
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
