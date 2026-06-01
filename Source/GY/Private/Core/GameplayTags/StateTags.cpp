#include "Core/GameplayTags/StateTags.h"

namespace GYStateTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Life_Alive, "State.Life.Alive");
	UE_DEFINE_GAMEPLAY_TAG(State_Life_Downed, "State.Life.Downed");
	UE_DEFINE_GAMEPLAY_TAG(State_Life_BeingRevived, "State.Life.BeingRevived");
	UE_DEFINE_GAMEPLAY_TAG(State_Life_Dead, "State.Life.Dead");

	UE_DEFINE_GAMEPLAY_TAG(State_Hit_Stagger, "State.Hit.Stagger");
	UE_DEFINE_GAMEPLAY_TAG(State_Hit_KnockDown, "State.Hit.KnockDown");
	UE_DEFINE_GAMEPLAY_TAG(State_Hit_Stun, "State.Hit.Stun");

	UE_DEFINE_GAMEPLAY_TAG(State_Exhausted, "State.Exhausted");


	UE_DEFINE_GAMEPLAY_TAG(State_Combat_InCombat, "State.Combat.InCombat");
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_SuperArmor, "State.Combat.SuperArmor");

	UE_DEFINE_GAMEPLAY_TAG(Action_Reviving, "Action.Reviving");

	UE_DEFINE_GAMEPLAY_TAG(State_Interaction_TimeRift, "State.Interaction.TimeRift");
	UE_DEFINE_GAMEPLAY_TAG(State_Interaction_TimeRift_Altar, "State.Interaction.TimeRift.Altar");
	UE_DEFINE_GAMEPLAY_TAG(State_Interaction_TimeRift_Reroll, "State.Interaction.TimeRift.Reroll");


}
