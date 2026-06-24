#pragma once

#include "NativeGameplayTags.h"

namespace GYEffectTags
{
	// GameplayEffect 분류용 메타 태그.
	// 각 GE의 Asset Tag 또는 Granted Tag 로 박아두면,
	// Granted Application Immunity Tags 매칭으로 일괄 차단 가능.

	// 군중 제어(Crowd Control) 계열 효과. Stagger/Stun/KnockDown 등.
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_CC)
}
