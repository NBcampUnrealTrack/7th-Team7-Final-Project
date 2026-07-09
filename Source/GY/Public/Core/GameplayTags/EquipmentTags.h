#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// Equipment Slot
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Weapon)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Outfit)        // 방어구 (상의)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Helmet)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Accessory1)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Accessory2)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Accessory3)

	// Weapon Type
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Sword)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Greatsword)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Unarmed)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_SwordAndShield)

	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Part_Body)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Part_Foot)
}
