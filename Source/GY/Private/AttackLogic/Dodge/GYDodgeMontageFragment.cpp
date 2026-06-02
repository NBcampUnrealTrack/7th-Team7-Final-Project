#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYDodgeMontageFragment::UGYDodgeMontageFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_DodgeMontage;
}

const FGYDodgeMontageSet* UGYDodgeMontageFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYDodgeMontageSet* DefaultResult = nullptr;

	for (const auto& Pair : MontageAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			if (Pair.Value.DodgeMontage)
				DefaultResult = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.DodgeMontage ? &Pair.Value : nullptr;
		}
	}

	return DefaultResult;
}
