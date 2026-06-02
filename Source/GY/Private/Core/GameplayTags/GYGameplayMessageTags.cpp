#include "Core/GameplayTags/GYGameplayMessageTags.h"

namespace GYGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Message_Combat_DamageDealt, "GY.Message.Combat.DamageDealt");
    UE_DEFINE_GAMEPLAY_TAG(Message_Combat_DamageTaken, "GY.Message.Combat.DamageTaken");
    UE_DEFINE_GAMEPLAY_TAG(Message_Combat_Crit, "GY.Message.Combat.Crit");
    UE_DEFINE_GAMEPLAY_TAG(Message_Combat_ParrySuccess, "GY.Message.Combat.ParrySuccess");
    UE_DEFINE_GAMEPLAY_TAG(Message_Combat_BlockSuccess, "GY.Message.Combat.BlockSuccess");
    UE_DEFINE_GAMEPLAY_TAG(Message_Combat_KnockDown, "GY.Message.Combat.KnockDown");

    UE_DEFINE_GAMEPLAY_TAG(Message_Player_Died, "GY.Message.Player.Died");
    UE_DEFINE_GAMEPLAY_TAG(Message_Player_Revived, "GY.Message.Player.Revived");
    UE_DEFINE_GAMEPLAY_TAG(Message_Player_Respawned, "GY.Message.Player.Respawned");
    UE_DEFINE_GAMEPLAY_TAG(Message_Player_LevelUp, "GY.Message.Player.LevelUp");

    UE_DEFINE_GAMEPLAY_TAG(Message_Inventory_ItemAdded, "GY.Message.Inventory.ItemAdded");
    UE_DEFINE_GAMEPLAY_TAG(Message_Inventory_ItemRemoved, "GY.Message.Inventory.ItemRemoved");
    UE_DEFINE_GAMEPLAY_TAG(Message_Inventory_ItemEquipped, "GY.Message.Inventory.ItemEquipped");
    UE_DEFINE_GAMEPLAY_TAG(Message_Inventory_PotionUsed, "GY.Message.Inventory.PotionUsed");
    UE_DEFINE_GAMEPLAY_TAG(Message_Inventory_PotionRecharged, "GY.Message.Inventory.PotionRecharged");
    UE_DEFINE_GAMEPLAY_TAG(Message_Inventory_PotionSlotChanged, "GY.Message.Inventory.PotionSlotChanged");
    UE_DEFINE_GAMEPLAY_TAG(Message_Inventory_EntryChanged, "GY.Message.Inventory.EntryChanged");

    UE_DEFINE_GAMEPLAY_TAG(Message_Equipment_LoadoutSlotChanged, "GY.Message.Equipment.LoadoutSlotChanged");

    UE_DEFINE_GAMEPLAY_TAG(Message_Build_NodeUnlocked, "GY.Message.Build.NodeUnlocked");
    UE_DEFINE_GAMEPLAY_TAG(Message_Build_ActionToggled, "GY.Message.Build.ActionToggled");

    UE_DEFINE_GAMEPLAY_TAG(Message_Quest_Started, "GY.Message.Quest.Started");
    UE_DEFINE_GAMEPLAY_TAG(Message_Quest_Progressed, "GY.Message.Quest.Progressed");
    UE_DEFINE_GAMEPLAY_TAG(Message_Quest_Completed, "GY.Message.Quest.Completed");
    UE_DEFINE_GAMEPLAY_TAG(Message_Region_Entered, "GY.Message.Region.Entered");

    UE_DEFINE_GAMEPLAY_TAG(Message_Party_MemberJoined, "GY.Message.Party.MemberJoined");
    UE_DEFINE_GAMEPLAY_TAG(Message_Party_MemberLeft, "GY.Message.Party.MemberLeft");
    UE_DEFINE_GAMEPLAY_TAG(Message_Party_MemberDied, "GY.Message.Party.MemberDied");
    UE_DEFINE_GAMEPLAY_TAG(Message_Chat_Received, "GY.Message.Chat.Received");

	UE_DEFINE_GAMEPLAY_TAG(Message_Interaction_OptionsChanged, "GY.Message.Interaction.OptionsChanged");

	UE_DEFINE_GAMEPLAY_TAG(Message_UI_XPProgress, "Message.UI.XPProgress");
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_PlayerName, "Message.UI.PlayerName");

	UE_DEFINE_GAMEPLAY_TAG(Message_Stat_Health, "Message.Stat.Health");
	UE_DEFINE_GAMEPLAY_TAG(Message_Stat_Stamina, "Message.Stat.Stamina");
	UE_DEFINE_GAMEPLAY_TAG(Message_Stat_Poise, "Message.Stat.Poise");

	UE_DEFINE_GAMEPLAY_TAG(Message_Character_Ready, "Message.Character.Ready");

	UE_DEFINE_GAMEPLAY_TAG(Message_World_TimeChanged, "Message.World.TimeChanged");
}
