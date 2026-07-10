#include "Core/GameplayTags/SoundTags.h"

namespace GYGameplayTags
{
	/* 플레이어 사운드 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Walk, "Sound.Player.Walk")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Run, "Sound.Player.Run")

	// 약공격
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Light, "Sound.Player.Attack.Light")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Stab, "Sound.Player.Attack.Stab")

	// 강공격
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Heavy_Charge, "Sound.Player.Attack.Heavy.Charge")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Heavy_Release, "Sound.Player.Attack.Heavy.Release")

	// 패링
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Parry_Attempt, "Sound.Player.Parry.Attempt")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Parry_Success, "Sound.Player.Parry.Success")

	// 방패 블로킹
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Block_Success, "Sound.Player.Block.Success")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Block_Break, "Sound.Player.Block.Break")

	// 회피
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Dodge, "Sound.Player.Dodge")

	// 물약
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Potion, "Sound.Player.Potion")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Healing, "Sound.Player.Healing")

	// 공격 적중
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Hit_Light, "Sound.Player.Hit.Light")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Hit_Heavy, "Sound.Player.Hit.Heavy")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Hit_Critical, "Sound.Player.Hit.Critical")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Hit_ParryCounter, "Sound.Player.Hit.ParryCounter")

	// 플레이어 피격
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Damage_Light, "Sound.Player.Damage.Light")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Damage_Heavy, "Sound.Player.Damage.Heavy")

	// 경직
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Stagger, "Sound.Player.Stagger")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Stun, "Sound.Player.Stun")

	/* 적 사운드 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Enemy_Regen, "Sound.Enemy.Regen")


	//펭마오
	UE_DEFINE_GAMEPLAY_TAG(Sound_Feng_Attack_1, "Sound.Feng.Attack.1") //휘두르기
	UE_DEFINE_GAMEPLAY_TAG(Sound_Feng_Attack_2, "Sound.Feng.Attack.2") //찍기
	UE_DEFINE_GAMEPLAY_TAG(Sound_Feng_Attack_3, "Sound.Feng.Attack.3") //두번휘두르기
	UE_DEFINE_GAMEPLAY_TAG(Sound_Feng_Attack_4, "Sound.Feng.Attack.4") //목소리
	UE_DEFINE_GAMEPLAY_TAG(Sound_Feng_Jump_Up, "Sound.Feng.Jump.Up") //목소리
	UE_DEFINE_GAMEPLAY_TAG(Sound_Feng_Death, "Sound.Feng.Death") //목소리

	//스패로우
	UE_DEFINE_GAMEPLAY_TAG(Sound_Sparrow_Attack, "Sound.Sparrow.Attack") //발사
	UE_DEFINE_GAMEPLAY_TAG(Sound_Sparrow_Death, "Sound.Sparrow.Death") //목소리

	//보스
	UE_DEFINE_GAMEPLAY_TAG(Sound_Boss_Impact, "Sound.Boss.Impact") //터지는 소리
	UE_DEFINE_GAMEPLAY_TAG(Sound_Boss_Seed, "Sound.Boss.Seed") //씨앗날리는거
	UE_DEFINE_GAMEPLAY_TAG(Sound_Boss_Poison, "Sound.Boss.Poison") //독 퍼지는
	UE_DEFINE_GAMEPLAY_TAG(Sound_Boss_Bloom, "Sound.Boss.Bloom") //떨어지는
	UE_DEFINE_GAMEPLAY_TAG(Sound_Boss_Roar, "Sound.Boss.Roar")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Boss_Vine, "Sound.Boss.Vine") //페이즈2
	UE_DEFINE_GAMEPLAY_TAG(Sound_Boss_Pawn_Death, "Sound.Boss.Pawn.Death")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Boss_Pawn_Attack, "Sound.Boss.Pawn.Attack")

	//중간보스

	/* 상호 작용 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_Open, "Sound.Interaction.LootBox.Open")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_Close, "Sound.Interaction.LootBox.Close")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_First, "Sound.Interaction.LootBox.First")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_Door_Open, "Sound.Interaction.Door.Open")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_Door_Close, "Sound.Interaction.Door.Close")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_TimeRift, "Sound.Interaction.TimeRift")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_TimeRift_Rest, "Sound.Interaction.TimeRift.Rest")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_SecretDiary, "Sound.Interaction.SecretDiary")


	/* BGM */
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Combat, "Sound.BGM.Combat")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Boss, "Sound.BGM.Boss")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Lobby, "Sound.BGM.Lobby")

	/* Voice */
}
