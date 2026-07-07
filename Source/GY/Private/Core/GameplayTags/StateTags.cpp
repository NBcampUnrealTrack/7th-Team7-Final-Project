#include "Core/GameplayTags/StateTags.h"

namespace GYStateTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Life_Alive, "State.Life.Alive");
	UE_DEFINE_GAMEPLAY_TAG(State_Life_LowHP, "State.Life.LowHP");
	UE_DEFINE_GAMEPLAY_TAG(State_Life_Downed, "State.Life.Downed");
	UE_DEFINE_GAMEPLAY_TAG(State_Life_BeingRevived, "State.Life.BeingRevived");
	UE_DEFINE_GAMEPLAY_TAG(State_Life_Dead, "State.Life.Dead");

	UE_DEFINE_GAMEPLAY_TAG(State_Hit_Stagger, "State.Hit.Stagger");
	UE_DEFINE_GAMEPLAY_TAG(State_Hit_KnockDown, "State.Hit.KnockDown");
	UE_DEFINE_GAMEPLAY_TAG(State_Hit_Stun, "State.Hit.Stun");

	UE_DEFINE_GAMEPLAY_TAG(State_Exhausted, "State.Exhausted");
	UE_DEFINE_GAMEPLAY_TAG(State_Falling, "State.Falling");
	UE_DEFINE_GAMEPLAY_TAG(State_Climbing, "State.Climbing");

	UE_DEFINE_GAMEPLAY_TAG(State_Combat_InCombat, "State.Combat.InCombat");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_SuperArmor, "State.Combat.SuperArmor");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Invulnerable, "State.Combat.Invulnerable");

	UE_DEFINE_GAMEPLAY_TAG(State_LockOn, "State.LockOn")

	UE_DEFINE_GAMEPLAY_TAG(State_Regen_Delay_Stamina, "State.Regen.Delay.Stamina");
	UE_DEFINE_GAMEPLAY_TAG(State_Regen_Delay_Stagger, "State.Regen.Delay.Stagger");
	UE_DEFINE_GAMEPLAY_TAG(State_Regen_Delay_Stun, "State.Regen.Delay.Stun");

	UE_DEFINE_GAMEPLAY_TAG(State_Action_Reviving, "State.Action.Reviving");

	UE_DEFINE_GAMEPLAY_TAG(State_Special_LockOn_IgnoreRotation, "State.Special.LockOn.IgnoreRotation");

	UE_DEFINE_GAMEPLAY_TAG(State_Interaction_TimeRift, "State.Interaction.TimeRift");
	UE_DEFINE_GAMEPLAY_TAG(State_Interaction_TimeRift_Altar, "State.Interaction.TimeRift.Altar");
	UE_DEFINE_GAMEPLAY_TAG(State_Interaction_TimeRift_Enchant, "State.Interaction.TimeRift.Enchant");
	UE_DEFINE_GAMEPLAY_TAG(State_Interaction_TimeRift_SkillTree, "State.Interaction.TimeRift.SkillTree");

	UE_DEFINE_GAMEPLAY_TAG(State_Boss_Phase2, "State.Boss.Phase2")

	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Poison, "State.Debuff.Poison")

	UE_DEFINE_GAMEPLAY_TAG(State_ActivityPoints_Used, "State.ActivityPoints.Used")

}
