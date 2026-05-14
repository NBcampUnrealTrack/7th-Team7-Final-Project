#include "Core/GameplayTags/ItemTags.h"

namespace GYGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Equipment, "Item.Category.Equipment");
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Consumable, "Item.Category.Consumable");
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Material, "Item.Category.Material");
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Quest, "Item.Category.Quest");
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Gem, "Item.Category.Gem");

	UE_DEFINE_GAMEPLAY_TAG(Item_Grade_Normal, "Item.Grade.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Item_Grade_Special, "Item.Grade.Special");
	UE_DEFINE_GAMEPLAY_TAG(Item_Grade_Legendary, "Item.Grade.Legendary");

	UE_DEFINE_GAMEPLAY_TAG(Gem_Type_Inherent, "Gem.Type.Inherent");
	UE_DEFINE_GAMEPLAY_TAG(Gem_Type_Common, "Gem.Type.Common");

	UE_DEFINE_GAMEPLAY_TAG(Material_Tier_Normal, "Material.Tier.Normal");
	UE_DEFINE_GAMEPLAY_TAG(Material_Tier_Special, "Material.Tier.Special");
	UE_DEFINE_GAMEPLAY_TAG(Material_Tier_Legendary, "Material.Tier.Legendary");

	UE_DEFINE_GAMEPLAY_TAG(Consumable_ChargePool_HP, "Consumable.ChargePool.HP");
	UE_DEFINE_GAMEPLAY_TAG(Consumable_ChargePool_SP, "Consumable.ChargePool.SP");
	UE_DEFINE_GAMEPLAY_TAG(Consumable_ChargePool_FP, "Consumable.ChargePool.FP");
}
