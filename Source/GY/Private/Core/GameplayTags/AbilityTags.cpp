#include "Core/GameplayTags/AbilityTags.h"

namespace GYGameplayTags
{
	// Ability Type
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Combo, "Ability.Attack.Combo");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Charge, "Ability.Attack.Charge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Guard, "Ability.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Parry, "Ability.Parry");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "Ability.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Sprint, "Ability.Sprint");

	// Fragment Type
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Charge, "Ability.Fragment.Charge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Attack, "Ability.Fragment.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_ComboMontage, "Ability.Fragment.ComboMontage");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_ChargeMontage, "Ability.Fragment.ChargeMontage");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Collision, "Ability.Fragment.Collision");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Cancel, "Ability.Fragment.Cancel");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Parry, "Ability.Fragment.Parry");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_ParryMontage, "Ability.Fragment.ParryMontage");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Dodge, "Ability.Fragment.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_DodgeMontage, "Ability.Fragment.DodgeMontage");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Direction, "Ability.Fragment.Direction");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Sprint, "Ability.Fragment.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_Block, "Ability.Fragment.Block");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Fragment_BlockMontage, "Ability.Fragment.BlockMontage");
	// State
	UE_DEFINE_GAMEPLAY_TAG(Ability_State_Parrying, "Ability.State.Parrying");
	UE_DEFINE_GAMEPLAY_TAG(Ability_State_CancelWindow, "Ability.State.CancelWindow");
	UE_DEFINE_GAMEPLAY_TAG(Ability_State_Dodging, "Ability.State.Dodging");
	UE_DEFINE_GAMEPLAY_TAG(Ability_State_Sprint, "Ability.State.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Block, "Ability.Block");
	UE_DEFINE_GAMEPLAY_TAG(Ability_State_Blocking, "Ability.State.Blocking");
}
