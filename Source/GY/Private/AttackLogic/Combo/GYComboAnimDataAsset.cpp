#include "AttackLogic/Combo/GYComboAnimDataAsset.h"

const FComboAnimSet* UGYComboAnimDataAsset::GetBestMatchingAnimSet(
	const FGameplayTagContainer& OwnedTags, const FGameplayTag& FallbackTag) const
{
	const FComboAnimSet* EmptyKeyDefault = nullptr;

	for (const auto& Pair : ComboAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			EmptyKeyDefault = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return &Pair.Value;
		}
	}

	if (FallbackTag.IsValid())
	{
		if (const FComboAnimSet* FallbackSet = ComboAnimSets.Find(FallbackTag))
		{
			return FallbackSet;
		}
	}

	return EmptyKeyDefault;
}
