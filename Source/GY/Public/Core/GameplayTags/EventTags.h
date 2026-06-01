#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// Item events
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Item_Equipped)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Item_Unequipped)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Item_Enhanced)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Item_Enchanted)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Item_GemSocketed)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Item_Used)

	// Inventory events
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Inventory_ItemAdded)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Inventory_ItemRemoved)

	// Attack events
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Anim_Attack_DoTrace)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Anim_ComboWindowOpen)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Anim_ComboWindowClose)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Input_Attack)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Input_AttackRelease)

	// Parry events
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Parry_Hit)

	// Enemy Attack events
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Enemy_WeaponTrace_Hit)
}
