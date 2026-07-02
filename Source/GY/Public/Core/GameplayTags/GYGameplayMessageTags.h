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
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Inventory_PotionSlotChanged);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Inventory_EntryChanged);

	//제단
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Altar_EntryChanged);

	// 장비
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Equipment_LoadoutSlotChanged);

	// 룻박스
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Loot_ShowBox);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Loot_BoxStateChanged);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Loot_BoxOpened); // 게임 로직용 (DS 포함 서버 전용)

	// 무기 숙련도 빌드, 노드 관련 - 임시
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Build_NodeUnlocked);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Build_ActionToggled);

	// 퀘스트, 지역 - 임시
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Quest_Started);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Quest_Progressed);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Quest_Completed);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Quest_Event);
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
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PlayerName);

	// 아이템 정보 패널 (우클릭 → 표시). 인벤/루트/장비/인첸트 공용
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_ShowItemInfo);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PinItemInfo);

	// UI Attribute
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Stat_Health);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Stat_Stamina);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Stat_Poise);

	// 캐릭터 컴포넌트 초기화
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Character_Ready);

	// 세계 시간
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_World_TimeChanged);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_World_Reset);

	// 락온
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_LockOn_Changed);

	// 지역 나가기
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Region_Exited);

	// 보스
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Boss_State);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Boss_Stat_Health);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Boss_Stat_Poise);

	// 부활
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Player_RevivalProgress);

	// 엔딩 시퀀스
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Ending_Started);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Ending_CinematicFinished);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Ending_CreditsFinished);
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Ending_WaitingForPlayers);

	// 시네마틱 재생 상태
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Cinematic_State);

	// 설정 화면 토글
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_ToggleSettings);

	// 보스 광역기 시간
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Boss_AOETimer);

	// UI 시계 오버레이
	GY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_ClockOverlay);
}
