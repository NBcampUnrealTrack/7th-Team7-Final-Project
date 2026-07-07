#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// GameplayCue - 일단 임시 구조, 추후 수정 가능
	// 플레이어 피격
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Damage_Light);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Damage_Heavy);

	/* 피격 리액션 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_HitReaction);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Knockback);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Stagger);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Stun);

	/* 플레이어 공격 적중 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Hit_Light);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Hit_Heavy);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Hit_Critical);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Hit_ParryCounter);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Hit_ShockWave);

	/* 보스 액션 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Boss_Slam);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Boss_Roar);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Boss_Laser);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Boss_PhaseTransition);
	/* 플레이어 액션 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Player_Ultimate);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Run);

	// 약공격
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Attack_Light);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Attack_Stab);

	// 강공격
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Attack_Heavy_Charge);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Attack_Heavy_Release);

	// 패링
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Parry_Attempt);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Parry_Success);

	// 방패 블로킹
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Block_Success);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Block_Break);

	// 회피
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Dodge);

	// 물약
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Potion);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Healing_HP);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Player_Healing_SP);

	/* 상호 작용*/
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Interaction_LootBox);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Interaction_Door_Open);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Interaction_Door_Close);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Interaction_TimeRift);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Interaction_TimeRift_Rest);

	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Enemy_Regen);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Feng_Death);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Sparrow_Death);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Pawn_Death);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Boss_Impact);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Boss_Bloom);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Boss_Poison);

	/* 상태 이상 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Status_Stun);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Status_Stagger);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Status_Poison);

	/* 카메라 이펙트 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Camera_Zoom);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Camera_Shake);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Camera_Push);

	/* 텔포 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Enemy_Teleport_Disappear);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Enemy_Teleport_Appear);
}
