#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// Stat Modifier (SetByCaller 키)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Modifier_Deviation)              // ±5% 편차 주입 키
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Modifier_OptionMagnitude1)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Modifier_OptionMagnitude2)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Modifier_OptionMagnitude3)

	// 데미지 Execution(GE_Damage) SetByCaller 키
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_SetByCaller_MotionMultiplier)
}
