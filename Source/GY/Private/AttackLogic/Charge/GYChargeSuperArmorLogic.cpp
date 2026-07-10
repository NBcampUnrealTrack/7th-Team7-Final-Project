#include "AttackLogic/Charge/GYChargeSuperArmorLogic.h"
#include "Core/GameplayTags/AbilityTags.h"

//   ___ _____ _   _ ___
//  / __|_   _| | | | _ )
//  \__ \ | | | |_| | _ \
//  |___/ |_|  \___/|___/

void UGYChargeSuperArmorLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
}

void UGYChargeSuperArmorLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
}

TArray<FGameplayTag> UGYChargeSuperArmorLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_ChargeSuperArmor };
}
