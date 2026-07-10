#include "AttackLogic/Charge/GYChargeParryEffectFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYChargeParryEffectFragment::UGYChargeParryEffectFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ChargeParryEffect;
}

const FGYChargeParryEffectData* UGYChargeParryEffectFragment::GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const
{
	const FGYChargeParryEffectData* DefaultResult = nullptr;

	for (const auto& Pair : EffectSets)
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
