// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/GameplayTags/InputTag.h"

namespace GYGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");

	UE_DEFINE_GAMEPLAY_TAG(InputTag_Interact, "InputTag.Interact");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Exit, "InputTag.Exit");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Attack, "InputTag.Attack");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Charge, "InputTag.Charge");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Parry, "InputTag.Parry");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Dodge, "InputTag.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Sprint, "InputTag.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Parkour, "InputTag.Parkour");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Block, "InputTag.Block");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_LockOn, "InputTag.LockOn");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Revive, "InputTag.Revive");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_UI_ToggleSettings, "InputTag.UI.ToggleSettings");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_UI_NextTab, "InputTag.UI.NextTab");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_UI_PrevTab, "InputTag.UI.PrevTab");
}
