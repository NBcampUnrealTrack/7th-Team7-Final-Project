#include "AttackLogic/Charge/GYChargeFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYChargeFragment::UGYChargeFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_Charge;
}

const FGYChargeData* UGYChargeFragment::GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const
{
	const FGYChargeData* DefaultResult = nullptr;

	for (const auto& Pair : ChargeDataSets)
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
