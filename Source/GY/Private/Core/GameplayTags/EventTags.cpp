#include "Core/GameplayTags/EventTags.h"

namespace GYGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Equipped,    "Event.Item.Equipped");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Unequipped,  "Event.Item.Unequipped");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Enhanced,    "Event.Item.Enhanced");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Enchanted,   "Event.Item.Enchanted");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_GemSocketed, "Event.Item.GemSocketed");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Used,        "Event.Item.Used");

	UE_DEFINE_GAMEPLAY_TAG(Event_Inventory_ItemAdded,   "Event.Inventory.ItemAdded");
	UE_DEFINE_GAMEPLAY_TAG(Event_Inventory_ItemRemoved, "Event.Inventory.ItemRemoved");
}
