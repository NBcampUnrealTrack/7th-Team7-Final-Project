#include "AttackLogic/Charge/GYChargeSuperArmorFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

//   ___ _____ _   _ ___
//  / __|_   _| | | | _ )
//  \__ \ | | | |_| | _ \
//  |___/ |_|  \___/|___/

UGYChargeSuperArmorFragment::UGYChargeSuperArmorFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_ChargeSuperArmor;
}

bool UGYChargeSuperArmorFragment::IsActive(const FGameplayTagContainer& OwnedTags) const
{
	return RequiredTags.IsEmpty() || OwnedTags.HasAny(RequiredTags);
}
