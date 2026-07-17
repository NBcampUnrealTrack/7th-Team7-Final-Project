#include "Core/GameplayTags/SoundTags.h"

namespace GYGameplayTags
{
	/* 플레이어 사운드 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Walk_Dirt, "Sound.Player.Walk.Dirt")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Walk_Wood, "Sound.Player.Walk.Wood")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Walk_Grass, "Sound.Player.Walk.Grass")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Walk_Stone, "Sound.Player.Walk.Stone")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Run_Dirt, "Sound.Player.Run.Dirt")

	// 약공격
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Light, "Sound.Player.Attack.Light")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Stab, "Sound.Player.Attack.Stab")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Light_Great, "Sound.Player.Attack.Light_Great")

	// 강공격
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Heavy_Charge, "Sound.Player.Attack.Heavy.Charge")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Heavy_Release, "Sound.Player.Attack.Heavy.Release")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Attack_Heavy_Release_Great, "Sound.Player.Attack.Heavy.Release_Great")

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

	//챕터보스
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_GreatSword_1, "Sound.ChapterBoss.GreatSword.1")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_GreatSword_2, "Sound.ChapterBoss.GreatSword.2")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_GreatSword_3, "Sound.ChapterBoss.GreatSword.3")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_GreatSword_4, "Sound.ChapterBoss.GreatSword.4")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_GreatSword_5, "Sound.ChapterBoss.GreatSword.5")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_Sword_1, "Sound.ChapterBoss.Sword.1")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_Sword_2, "Sound.ChapterBoss.Sword.2")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_Sword_3, "Sound.ChapterBoss.Sword.3")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_Sword_4, "Sound.ChapterBoss.Sword.4")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_Blade, "Sound.ChapterBoss.Blade")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_LongDash, "Sound.ChapterBoss.LongDash")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_Dash, "Sound.ChapterBoss.Dash")
	UE_DEFINE_GAMEPLAY_TAG(Sound_ChapterBoss_Death, "Sound.ChapterBoss.Death")

	//모리게시
	UE_DEFINE_GAMEPLAY_TAG(Sound_Morigesh_ElectricMark, "Sound.Morigesh.ElectricMark")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Morigesh_Lightning, "Sound.Morigesh.Lightning")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Morigesh_Orb, "Sound.Morigesh.Orb")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Morigesh_Knife, "Sound.Morigesh.Knife")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Morigesh_Ultimate, "Sound.Morigesh.Ultimate")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Morigesh_Death, "Sound.Morigesh.Death")


	//스코치 Scorch
	UE_DEFINE_GAMEPLAY_TAG(Sound_Scorch_Run, "Sound.Scorch.Run")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Scorch_Eat, "Sound.Scorch.Eat")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Scorch_Jump, "Sound.Scorch.Jump")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Scorch_Bomb, "Sound.Scorch.Bomb")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Scorch_Pop, "Sound.Scorch.Pop")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Scorch_Fire, "Sound.Scorch.Fire")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Scorch_Death, "Sound.Scorch.Death")


	//그래이스톤
	UE_DEFINE_GAMEPLAY_TAG(Sound_GreyStone_Death, "Sound.GreyStone.Death")
	UE_DEFINE_GAMEPLAY_TAG(Sound_GreyStone_Attack_1, "Sound.GreyStone.Attack.1")
	UE_DEFINE_GAMEPLAY_TAG(Sound_GreyStone_Attack_2, "Sound.GreyStone.Attack.2")
	UE_DEFINE_GAMEPLAY_TAG(Sound_GreyStone_Dash, "Sound.GreyStone.Dash")
	UE_DEFINE_GAMEPLAY_TAG(Sound_GreyStone_Voice, "Sound.GreyStone.Voice")

	/* 상호 작용 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_Open, "Sound.Interaction.LootBox.Open")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_Close, "Sound.Interaction.LootBox.Close")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_First, "Sound.Interaction.LootBox.First")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_Door_Open, "Sound.Interaction.Door.Open")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_Door_Close, "Sound.Interaction.Door.Close")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_TimeRift, "Sound.Interaction.TimeRift")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_TimeRift_Rest, "Sound.Interaction.TimeRift.Rest")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_TimeRift_Enchant, "Sound.Interaction.TimeRift.Enchant")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_TimeRift_Altar, "Sound.Interaction.TimeRift.Altar")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_SecretDiary, "Sound.Interaction.SecretDiary")


	UE_DEFINE_GAMEPLAY_TAG(Sound_Quest_Completed, "Sound.Quest.Completed")
	UE_DEFINE_GAMEPLAY_TAG(Sound_World_Reset, "Sound.World.Reset")
	UE_DEFINE_GAMEPLAY_TAG(Sound_World_Clock, "Sound.World.Clock")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Item_Looting, "Sound.Item.Looting")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Level_Up, "Sound.Level.Up")

	/* 환경 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Ambient_Machine, "Sound.Ambient.Machine")

	/* BGM */
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Combat, "Sound.BGM.Combat")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Lobby, "Sound.BGM.Lobby")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Area1, "Sound.BGM.Area1")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Area2, "Sound.BGM.Area2")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_FinalBoss, "Sound.BGM.FinalBoss")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_RegionBoss, "Sound.BGM.RegionBoss")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Village, "Sound.BGM.Village")


	/* Voice */
}
