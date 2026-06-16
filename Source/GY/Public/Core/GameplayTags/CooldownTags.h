#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	/* Consumable Items */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Consumable)

	/* 인첸트 온히트 효과 발동 쿨타임 (효과별 독립) */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Bleed)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_DefBreak)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Lifesteal)

}
