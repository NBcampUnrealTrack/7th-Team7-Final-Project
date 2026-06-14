#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	/* 플레이어 사운드 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Walk)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Run)

	// 약공격
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Attack_Light);

	// 강공격
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Attack_Heavy_Charge);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Attack_Heavy_Release);

	// 패링
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Parry_Attempt);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Parry_Success);

	// 방패 블로킹
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Block_Success);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Block_Break);

	// 회피
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Dodge);

	// 물약
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Potion);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Player_Healing);

	/* 적 사운드 */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Goblin_Roar)

	/* 상호 작용*/
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_LootBox_Close)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_LootBox_Open)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_LootBox_First)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_Door_Open)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_Door_Close)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_TimeRift)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_Interaction_TimeRift_Rest)

	/* BGM */
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_BGM_Combat)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_BGM_Boss)
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sound_BGM_Lobby)

	/* Voice */
}
