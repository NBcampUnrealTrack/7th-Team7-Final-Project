#include "AttackLogic/Charge/GYChargeInfiniteFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYChargeInfiniteFragment::UGYChargeInfiniteFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ChargeInfinite;
}

bool UGYChargeInfiniteFragment::IsActive(const FGameplayTagContainer& OwnedTags) const
{
	return RequiredTags.IsEmpty() || OwnedTags.HasAny(RequiredTags);
}
