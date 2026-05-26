#include "AttackLogic/Combo/GYComboFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYComboFragment::UGYComboFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_Attack;
}

const TArray<FGYComboStepData>* UGYComboFragment::GetBestMatchingSteps(const FGameplayTagContainer& OwnedTags) const
{
	const TArray<FGYComboStepData>* DefaultResult = nullptr;

	for (const auto& Pair : ComboSteps)
	{
		if (!Pair.Key.IsValid())
		{
			if (!Pair.Value.Steps.IsEmpty())
				DefaultResult = &Pair.Value.Steps;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.Steps.IsEmpty() ? nullptr : &Pair.Value.Steps;
		}
	}

	return DefaultResult;
}
