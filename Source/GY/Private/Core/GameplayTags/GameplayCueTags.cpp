#include "Core/GameplayTags/GameplayCueTags.h"

namespace GYGameplayTags
{

	/* 피격 리액션 */
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_HitReaction, "GameplayCue.Combat.HitReaction");

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
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_Player_Ultimate, "GameplayCue.Combat.Player.Ultimate");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Run, "GameplayCue.Player.Run");

	// 약공격
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Attack_Light, "GameplayCue.Player.Attack.Light");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Attack_Stab, "GameplayCue.Player.Attack.Stab");

	// 강공격
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Attack_Heavy_Charge, "GameplayCue.Player.Attack.Heavy.Charge");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Attack_Heavy_Release, "GameplayCue.Player.Attack.Heavy.Release");

	// 패링
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Parry_Attempt, "GameplayCue.Player.Parry.Attempt");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Parry_Success, "GameplayCue.Player.Parry.Success");

	// 방패 블로킹
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Block_Success, "GameplayCue.Player.Block.Success");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Block_Break, "GameplayCue.Player.Block.Break");

	// 회피
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Dodge, "GameplayCue.Player.Dodge");

	// 물약
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Potion, "GameplayCue.Player.Potion");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Healing_HP, "GameplayCue.Player.Healing.HP");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Healing_SP, "GameplayCue.Player.Healing.SP");

	// 상호 작용
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_LootBox, "GameplayCue.Interaction.LootBox");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_Door_Open, "GameplayCue.Interaction.Door.Open");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_Door_Close, "GameplayCue.Interaction.Door.Close");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_TimeRift, "GameplayCue.Interaction.TimeRift");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_TimeRift_Rest, "GameplayCue.Interaction.TimeRift.Rest");


	// 상태 이상
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Stun, "GameplayCue.Status.Stun");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Burn, "GameplayCue.Status.Burn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Frozen, "GameplayCue.Status.Frozen");

	// 카메라 이펙트
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Zoom, "GameplayCue.Camera.Zoom");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Shake, "GameplayCue.Camera.Shake");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Push, "GameplayCue.Camera.Push");

	// 텔포
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Enemy_Teleport_Disappear, "GameplayCue.Enemy.Teleport.Disappear");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Enemy_Teleport_Appear, "GameplayCue.Enemy.Teleport.Appear");
}
