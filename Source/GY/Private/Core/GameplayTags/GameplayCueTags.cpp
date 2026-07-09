#include "Core/GameplayTags/GameplayCueTags.h"

namespace GYGameplayTags
{
	// 플레이어 피격
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Damage_Light, "GameplayCue.Player.Damage.Light")
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Damage_Heavy, "GameplayCue.Player.Damage.Heavy")

	/* 피격 리액션 */
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Combat_HitReaction, "GameplayCue.Combat.HitReaction");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Stagger, "GameplayCue.Player.Stagger")
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Stun, "GameplayCue.Player.Stun")

	// 플레이어 공격 적중
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Hit_Light, "GameplayCue.Player.Hit.Light");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Hit_Heavy, "GameplayCue.Player.Hit.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Hit_Critical, "GameplayCue.Player.Hit.Critical");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Hit_ParryCounter, "GameplayCue.Player.Hit.ParryCounter");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Player_Hit_ShockWave, "GameplayCue.Player.Hit.ShockWave");

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
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Interaction_SecretDiary, "GameplayCue.Interaction.SecretDiary");

	// 적 리젠
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Enemy_Regen, "GameplayCue.Enemy.Regen");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Feng_Death, "GameplayCue.Feng.Death");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Sparrow_Death, "GameplayCue.Sparrow.Death");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Pawn_Death, "GameplayCue.Pawn.Death");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Boss_Impact, "GameplayCue.Boss.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Boss_Bloom, "GameplayCue.Boss.Bloom");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Boss_Poison, "GameplayCue.Boss.Poison");

	// 상태 이상 - 포스트 프로세스
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Stun, "GameplayCue.Status.Stun");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Stagger, "GameplayCue.Status.Stagger");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Status_Poison, "GameplayCue.Status.Poison");

	// 카메라 이펙트
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Zoom, "GameplayCue.Camera.Zoom");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Shake, "GameplayCue.Camera.Shake");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Camera_Push, "GameplayCue.Camera.Push");

	// 텔포
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Enemy_Teleport_Disappear, "GameplayCue.Enemy.Teleport.Disappear");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Enemy_Teleport_Appear, "GameplayCue.Enemy.Teleport.Appear");
}
