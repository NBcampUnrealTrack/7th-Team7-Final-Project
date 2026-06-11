#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// Stat Modifier (SetByCaller 키)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Modifier_Deviation)              // ±5% 편차 주입 키
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Modifier_OptionMagnitude1)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Modifier_OptionMagnitude2)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Modifier_OptionMagnitude3)

	// 타격 GE(GE_HitImpact) SetByCaller 키
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitImpact_SetByCaller_MotionMultiplier)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitImpact_SetByCaller_StaggerAmount)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitImpact_SetByCaller_StunAmount)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitImpact_SetByCaller_BlockReduction)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitImpact_SetByCaller_BlockHitCostMultiplier)
}
