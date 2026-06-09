#include "Core/GameplayTags/SoundTags.h"

namespace GYGameplayTags
{
	/* 플레이어 사운드 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Walk, "Sound.Player.Walk")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Player_Run, "Sound.Player.Run")

	/* 적 사운드 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Goblin_Roar, "Sound.Goblin.Roar")


	/* 상호 작용 */
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_Open, "Sound.Interaction.LootBox.Open")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_Close, "Sound.Interaction.LootBox.Close")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_LootBox_First, "Sound.Interaction.LootBox.First")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_Door_Open, "Sound.Interaction.Door.Open")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_Door_Close, "Sound.Interaction.Door.Close")
	UE_DEFINE_GAMEPLAY_TAG(Sound_Interaction_TimeRift, "Sound.Interaction.TimeRift")

	/* BGM */
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Combat, "Sound.BGM.Combat")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Boss, "Sound.BGM.Boss")
	UE_DEFINE_GAMEPLAY_TAG(Sound_BGM_Lobby, "Sound.BGM.Lobby")

	/* Voice */
}
