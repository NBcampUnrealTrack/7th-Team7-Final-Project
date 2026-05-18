#pragma once

#include "NativeGameplayTags.h"

namespace GYUILayerTags
{
	GYUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);      // HUD 상시 표시
	GYUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);      // 풀스크린 메뉴 - 인벤토리, 무기 빌드, 옵션
	GYUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);     // 최상위 확인창
}
