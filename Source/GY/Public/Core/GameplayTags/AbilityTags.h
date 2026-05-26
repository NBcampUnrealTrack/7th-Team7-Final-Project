#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// Ability Type
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Combo)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Charge)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Parry)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge)

	// Fragment Type
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fragment_Charge)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fragment_Attack)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fragment_ComboMontage)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fragment_ChargeMontage)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fragment_Collision)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fragment_Cancel)

}
