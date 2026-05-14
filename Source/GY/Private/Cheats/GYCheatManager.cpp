#include "Cheats/GYCheatManager.h"

#include "Equipment/EquipmentLoadoutComponent.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"

namespace
{
	AGYPlayerState* GetGYPlayerState(const UCheatManager* CheatManager)
	{
		APlayerController* PC = CheatManager->GetOuterAPlayerController();
		if (!IsValid(PC)) return nullptr;
		return PC->GetPlayerState<AGYPlayerState>();
	}
}

void UGYCheatManager::GY_AddItem(const FString& ItemPath, int32 Count)
{
	Server_AddItem(ItemPath, Count);
}

void UGYCheatManager::GY_EquipItem(const FString& ItemPath)
{
	Server_EquipItem(ItemPath);
}

void UGYCheatManager::GY_UnequipSlot(const FString& SlotTagName)
{
	const FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(*SlotTagName), false);
	if (!SlotTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_UnequipSlot: invalid tag '%s'"), *SlotTagName);
		return;
	}
	Server_UnequipSlot(SlotTag);
}

void UGYCheatManager::GY_PrintInventory()
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	UE_LOG(LogTemp, Log, TEXT("=== Inventory ==="));
	int32 Index = 0;
	for (const FInventoryEntry& Entry : Inv->GetEntries())
	{
		UItemDefinition* Def = Entry.Definition.LoadSynchronous();
		const FName ItemId = IsValid(Def) ? Def->ItemId : NAME_None;
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s x%d (InstanceId=%s)"),
			Index, *ItemId.ToString(), Entry.StackCount, *Entry.InstanceId.ToString());
		++Index;
	}
}

void UGYCheatManager::GY_PrintLoadout()
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Loadout)) return;

	UE_LOG(LogTemp, Log, TEXT("=== Loadout ==="));
	for (const FEquipmentLoadoutEntry& Entry : Loadout->GetEntries())
	{
		UE_LOG(LogTemp, Log, TEXT("  %s -> %s"),
			*Entry.SlotTag.ToString(), *Entry.InstanceId.ToString());
	}
}

void UGYCheatManager::Server_AddItem_Implementation(const FString& ItemPath, int32 Count)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	UItemDefinition* Def = LoadObject<UItemDefinition>(nullptr, *ItemPath);
	if (!IsValid(Def))
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_AddItem: failed to load %s"), *ItemPath);
		return;
	}

	FGuid OutId;
	if (Inv->TryAddItem(Def, Count, OutId))
	{
		UE_LOG(LogTemp, Log, TEXT("Server_AddItem: %s x%d (InstanceId=%s)"),
			*Def->ItemId.ToString(), Count, *OutId.ToString());
	}
}

void UGYCheatManager::Server_EquipItem_Implementation(const FString& ItemPath)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Inv) || !IsValid(Loadout)) return;

	UItemDefinition* Def = LoadObject<UItemDefinition>(nullptr, *ItemPath);
	if (!IsValid(Def))
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_EquipItem: failed to load %s"), *ItemPath);
		return;
	}

	FGuid OutId;
	if (!Inv->TryAddItem(Def, 1, OutId)) return;

	Loadout->Server_RequestEquip(OutId);
}

void UGYCheatManager::Server_UnequipSlot_Implementation(FGameplayTag SlotTag)
{
	AGYPlayerState* PS = GetGYPlayerState(this);
	if (!IsValid(PS)) return;

	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Loadout)) return;

	Loadout->Server_RequestUnequip(SlotTag);
}
