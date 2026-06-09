#include "AttackLogic/SDisable/GYSDisableMontageFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYSDisableMontageFragment::UGYSDisableMontageFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_SDisableMontage;
}

const FGYSDisableMontageSet* UGYSDisableMontageFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYSDisableMontageSet* DefaultResult = nullptr;

	for (const auto& Pair : MontageAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			if (Pair.Value.LoopMontage)
				DefaultResult = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.LoopMontage ? &Pair.Value : nullptr;
		}
	}

	return DefaultResult;
}
