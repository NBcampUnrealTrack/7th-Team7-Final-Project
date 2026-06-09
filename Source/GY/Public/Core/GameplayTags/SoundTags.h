#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	/* 플레이어 사운드 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Walk)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Run)

	/* 적 사운드 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Goblin_Roar)

	/* 상호 작용*/
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_LootBox_Close)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_LootBox_Open)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_LootBox_First)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_Door_Open)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_Door_Close)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_TimeRift)

	/* BGM */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_BGM_Combat)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_BGM_Boss)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_BGM_Lobby)

	/* Voice */
}
