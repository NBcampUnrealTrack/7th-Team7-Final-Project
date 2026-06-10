#include "Core/GameplayTags/GameplayCueTags.h"

namespace GYGameplayTags
{
	// 플레이어 피격
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Hit_Light, "GameplayCue.Combat.Hit.Light");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Hit_Heavy, "GameplayCue.Combat.Hit.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Hit_Critical, "GameplayCue.Combat.Hit.Critical");

	// 보스 액션
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Boss_Slam, "GameplayCue.Combat.Boss.Slam");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Boss_Roar, "GameplayCue.Combat.Boss.Roar");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Boss_Laser, "GameplayCue.Combat.Boss.Laser");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Boss_PhaseTransition, "GameplayCue.Combat.Boss.PhaseTransition");

	// 플레이어 액션
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Player_Dash, "GameplayCue.Combat.Player.Dash");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Player_Parry, "GameplayCue.Combat.Player.Parry");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Player_Ultimate, "GameplayCue.Combat.Player.Ultimate");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Run, "GameplayCue.Player.Run");

	// 상호 작용
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_LootBox, "GameplayCue.Interaction.LootBox");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_Door_Open, "GameplayCue.Interaction.Door.Open");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_Door_Close, "GameplayCue.Interaction.Door.Close");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_TimeRift, "GameplayCue.Interaction.TimeRift");


	// 상태 이상
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Stun, "GameplayCue.Status.Stun");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Burn, "GameplayCue.Status.Burn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Frozen, "GameplayCue.Status.Frozen");

	// 카메라 이펙트
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Zoom, "GameplayCue.Camera.Zoom");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Shake, "GameplayCue.Camera.Shake");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Push, "GameplayCue.Camera.Push");
}
