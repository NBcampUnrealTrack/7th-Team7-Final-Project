#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// Item Category
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Equipment)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Consumable)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Material)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Quest)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Gem)

	// Item Grade (드랍 결과)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Grade_Normal)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Grade_Special)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Grade_Legendary)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Grade_Legendary_Engraved)

	// Gem Type
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gem_Type_Inherent)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gem_Type_Common)

	// Material Tier
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Tier_Normal)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Tier_Special)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Tier_Legendary)

	// Consumable Charge Pool (Consumable이 가리키는 풀)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Consumable_ChargePool_HP)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Consumable_ChargePool_SP)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Consumable_ChargePool_FP)
}
