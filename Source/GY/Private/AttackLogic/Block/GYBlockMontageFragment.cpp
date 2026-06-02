#include "AttackLogic/Block/GYBlockMontageFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYBlockMontageFragment::UGYBlockMontageFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_BlockMontage;
}

const FGYBlockMontageSet* UGYBlockMontageFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYBlockMontageSet* DefaultResult = nullptr;

	for (const auto& Pair : MontageAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			if (Pair.Value.HoldMontage)
				DefaultResult = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.HoldMontage ? &Pair.Value : nullptr;
		}
	}

	return DefaultResult;
}
