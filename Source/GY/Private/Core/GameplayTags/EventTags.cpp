#include "Core/GameplayTags/EventTags.h"

namespace GYGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Equipped, "Event.Item.Equipped");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Unequipped, "Event.Item.Unequipped");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Enhanced, "Event.Item.Enhanced");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Enchanted, "Event.Item.Enchanted");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_GemSocketed, "Event.Item.GemSocketed");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Used, "Event.Item.Used");

	UE_DEFINE_GAMEPLAY_TAG(Event_Inventory_ItemAdded, "Event.Inventory.ItemAdded");
	UE_DEFINE_GAMEPLAY_TAG(Event_Inventory_ItemRemoved, "Event.Inventory.ItemRemoved");

	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_Attack_DoTrace, "Event.Anim.Attack.DoTrace");
	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_ComboWindowOpen, "Event.Anim.Combo.WindowOpen");
	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_ComboWindowClose, "Event.Anim.Combo.WindowClose");
	UE_DEFINE_GAMEPLAY_TAG(Event_Input_Attack, "Event.Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Event_Input_AttackRelease, "Event.Input.AttackRelease");

	UE_DEFINE_GAMEPLAY_TAG(Event_Parry_Hit, "Event.Parry.Hit");

	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_WeaponTrace_Hit, "Event.Enemy.WeaponTrace.Hit");

	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Altar, "Event.TimeRift.Altar");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Rest, "Event.TimeRift.Rest");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Exit, "Event.TimeRift.Exit");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Reroll, "Event.TimeRift.Reroll");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_SkillTree, "Event.TimeRift.SkillTree");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_SkillTree_Exit, "Event.TimeRift.SkillTree.Exit");

	UE_DEFINE_GAMEPLAY_TAG(Event_SkillTree_Unlock, "Event.SkillTree.Unlock");

}
