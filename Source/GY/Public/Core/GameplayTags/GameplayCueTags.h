#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// GameplayCue - 일단 임시 구조, 추후 수정 가능

	/* 플레이어 피격 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Hit_Light);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Hit_Heavy);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Hit_Critical);
	/* 보스 액션 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Boss_Slam);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Boss_Roar);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Boss_Laser);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Boss_PhaseTransition);
	/* 플레이어 액션 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Player_Dash);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Player_Parry);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Player_Ultimate);
	/* 상태 이상 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Status_Stun);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Status_Burn);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Status_Frozen);
	/* 카메라 이펙트 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Camera_HitLight);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Camera_HitHeavy);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Camera_ZoomIn);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Camera_BossSlam);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Camera_Execution);
}
