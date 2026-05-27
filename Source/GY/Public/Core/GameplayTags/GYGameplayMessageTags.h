#pragma once

#include "NativeGameplayTags.h"

namespace GYGameplayTags
{
	// 전투 관련
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Combat_DamageDealt);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Combat_DamageTaken);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Combat_Crit);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Combat_ParrySuccess);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Combat_BlockSuccess);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Combat_KnockDown);

	// 플레이어 생명주기
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Player_Died);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Player_Revived);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Player_Respawned);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Player_LevelUp);

	// 인벤토리, 아이템 - 임시
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Inventory_ItemAdded);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Inventory_ItemRemoved);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Inventory_ItemEquipped);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Inventory_PotionUsed);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Inventory_PotionRecharged);

	// 무기 숙련도 빌드, 노드 관련 - 임시
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Build_NodeUnlocked);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Build_ActionToggled);

	// 퀘스트, 지역 - 임시
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Quest_Started);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Quest_Progressed);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Quest_Completed);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Region_Entered);

	// 멀티 관련 - 친구, 채팅
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Party_MemberJoined);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Party_MemberLeft);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Party_MemberDied);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Chat_Received);

	// 상호작용
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Interaction_OptionsChanged)

	// PlayerHUD
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_XPProgress);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PotionSlot);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_EquipmentSlot);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PlayerName);

	// UI Attribute
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Stat_Health);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Stat_Stamina);
}
