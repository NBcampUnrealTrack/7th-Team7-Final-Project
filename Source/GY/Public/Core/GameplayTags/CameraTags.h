#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// 카메라 제어 태그
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Camera_Mode_Exploration);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Camera_Mode_Boss);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Camera_Mode_Boss_Phase2);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Camera_Mode_Combat);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Camera_Mode_Cinematic);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Camera_Mode_ZoomIn);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Camera_Mode_ZoomOut);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Camera_Mode_Angle);
}
