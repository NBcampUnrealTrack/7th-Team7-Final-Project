#include "Core/GameplayTags/EventTags.h"

namespace GYGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Equipped, "Event.Item.Equipped");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Unequipped, "Event.Item.Unequipped");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Enhanced, "Event.Item.Enhanced");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Enchanted, "Event.Item.Enchanted");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_GemSocketed, "Event.Item.GemSocketed");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Used, "Event.Item.Used");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Used_HP, "Event.Item.Used.HP");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_Used_SP, "Event.Item.Used.SP");

	UE_DEFINE_GAMEPLAY_TAG(Event_Inventory_ItemAdded, "Event.Inventory.ItemAdded");
	UE_DEFINE_GAMEPLAY_TAG(Event_Inventory_ItemRemoved, "Event.Inventory.ItemRemoved");

	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_Attack_DoTrace, "Event.Anim.Attack.DoTrace");
	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_Attack_TraceBegin, "Event.Anim.Attack.TraceBegin");
	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_Attack_TraceEnd, "Event.Anim.Attack.TraceEnd");
	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_ComboWindowOpen, "Event.Anim.Combo.WindowOpen");
	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_ComboWindowClose, "Event.Anim.Combo.WindowClose");
	UE_DEFINE_GAMEPLAY_TAG(Event_Anim_TagApplyStart, "Event.Anim.TagApplyStart");
	UE_DEFINE_GAMEPLAY_TAG(Event_Input_Attack, "Event.Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Event_Input_AttackRelease, "Event.Input.AttackRelease");
	UE_DEFINE_GAMEPLAY_TAG(Event_Combo_StepStart, "Event.Combo.StepStart");
	UE_DEFINE_GAMEPLAY_TAG(Event_Combo_Advance, "Event.Combo.Advance");
	UE_DEFINE_GAMEPLAY_TAG(Event_Input_ParryRelease, "Event.Input.ParryRelease");

	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_Parkour_Execute, "Event.Ability.Parkour.Execute");
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_Dodge_Execute, "Event.Ability.Dodge.Execute");
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_Charge_Execute, "Event.Ability.Charge.Execute");
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_Cancelable, "Event.Ability.Cancelable");

	UE_DEFINE_GAMEPLAY_TAG(Event_Parry_Hit, "Event.Parry.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Parried, "Event.Parried");


	UE_DEFINE_GAMEPLAY_TAG(Event_JustGuard_Hit, "Event.JustGuard.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_JustGuarded, "Event.JustGuarded");

	UE_DEFINE_GAMEPLAY_TAG(Event_Block_Hit, "Event.Block.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Block_LoopEnd, "Event.Block.LoopEnd");

	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_WeaponTrace_Hit, "Event.Enemy.WeaponTrace.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_WeaponTrace_Begin, "Event.Enemy.WeaponTrace.Begin");
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_LaunchProjectile, "Event.Enemy.LaunchProjectile");
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_Slam_Land, "Event.Enemy.Slam.Land");
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_Teleport_Trigger, "Event.Enemy.Teleport.Trigger");
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_SpawnAreaDenialZones, "Event.Enemy.SpawnAreaDenialZones")
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_FireBreath_Tick, "Event.Enemy.FireBreath.Tick")
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_FireBreath_Start, "Event.Enemy.FireBreath.Start")
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_FireBreath_End, "Event.Enemy.FireBreath.End")
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_Combo_Branch, "Event.Enemy.Combo.Branch")
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_Marker_Arrived, "Event.Enemy.Marker.Arrived")
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_SummonPhantom, "Event.Enemy.SummonPhantom")

	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Exit, "Event.TimeRift.Exit");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Rest, "Event.TimeRift.Rest");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Altar, "Event.TimeRift.Altar");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Altar_Exit, "Event.TimeRift.Altar.Exit")
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Enchant, "Event.TimeRift.Enchant");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_Enchant_Exit, "Event.TimeRift.Enchant.Exit")
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_SkillTree, "Event.TimeRift.SkillTree");
	UE_DEFINE_GAMEPLAY_TAG(Event_TimeRift_SkillTree_Exit, "Event.TimeRift.SkillTree.Exit");

	UE_DEFINE_GAMEPLAY_TAG(Event_Ladder_ClimbRequest, "Event.Ladder.ClimbRequest");

	UE_DEFINE_GAMEPLAY_TAG(Event_SkillTree_Unlock, "Event.SkillTree.Unlock");

	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Stagger, "Event.Hit.Stagger");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Stun, "Event.Hit.Stun");

}
