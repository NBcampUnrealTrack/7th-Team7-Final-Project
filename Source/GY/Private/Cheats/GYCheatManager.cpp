#include "Cheats/GYCheatManager.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/Interactable.h"
#include "Interaction/InteractionOption.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Loot/LootBoxActor.h"
#include "Loot/LootTypes.h"
#include "Player/GYPlayerState.h"

namespace
{
	AGYPlayerState* GetGYPlayerState(const UCheatManager* CheatManager)
	{
		APlayerController* PC = CheatManager->GetOuterAPlayerController();
		if (!IsValid(PC)) return nullptr;
		return PC->GetPlayerState<AGYPlayerState>();
	}

	APawn* GetCheatPawn(const UCheatManager* CheatManager)
	{
		APlayerController* PC = CheatManager->GetOuterAPlayerController();
		return IsValid(PC) ? PC->GetPawn() : nullptr;
	}

	AActor* FindNearestInteractable(const UWorld* World, const FVector& Origin, float MaxDistance = 1000.f)
	{
		if (!IsValid(World)) return nullptr;

		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsWithInterface(World, UInteractable::StaticClass(), AllActors);

		AActor* Nearest = nullptr;
		float NearestDistSq = MaxDistance * MaxDistance;

		for (AActor* Actor : AllActors)
		{
			const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), Origin);
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Actor;
			}
		}
		return Nearest;
	}

	ALootBoxActor* FindNearestLootBox(const UWorld* World, const FVector& Origin, float MaxDistance = 1000.f)
	{
		if (!IsValid(World)) return nullptr;

		TArray<AActor*> AllBoxes;
		UGameplayStatics::GetAllActorsOfClass(World, ALootBoxActor::StaticClass(), AllBoxes);

		ALootBoxActor* Nearest = nullptr;
		float NearestDistSq = MaxDistance * MaxDistance;

		for (AActor* Actor : AllBoxes)
		{
			const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), Origin);
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Cast<ALootBoxActor>(Actor);
			}
		}
		return Nearest;
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

// ============================================================
// Loot / Interaction
// ============================================================

void UGYCheatManager::GY_SpawnLootBox(const FString& SourceId, const FString& LootTablePath)
{
	Server_SpawnLootBox(SourceId, LootTablePath);
}

void UGYCheatManager::GY_GetNearestInteractionOptions()
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	AActor* Nearest = FindNearestInteractable(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Nearest))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_GetNearestInteractionOptions: no interactable nearby"));
		return;
	}

	IInteractable* Interactable = Cast<IInteractable>(Nearest);
	if (Interactable == nullptr) return;

	TArray<FInteractionOption> Options;
	Interactable->GatherInteractionOptions(Pawn, Options);

	UE_LOG(LogTemp, Log, TEXT("=== Nearest Interactable: %s ==="), *Nearest->GetName());
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s (Tag=%s)"),
			i, *Options[i].Text.ToString(), *Options[i].OptionTag.ToString());
	}
}

void UGYCheatManager::GY_InvokeInteraction(const FString& OptionTagName)
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	const FGameplayTag OptionTag = FGameplayTag::RequestGameplayTag(FName(*OptionTagName), false);
	if (!OptionTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_InvokeInteraction: invalid tag '%s'"), *OptionTagName);
		return;
	}

	AActor* Nearest = FindNearestInteractable(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Nearest))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_InvokeInteraction: no interactable nearby"));
		return;
	}

	Server_InvokeInteraction(Nearest, OptionTag);
}

void UGYCheatManager::GY_PrintLootBoxContents()
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	ALootBoxActor* Box = FindNearestLootBox(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Box))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_PrintLootBoxContents: no loot box nearby"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("=== LootBox %s (Opened=%s) ==="),
		*Box->GetName(), Box->IsOpened() ? TEXT("true") : TEXT("false"));

	int32 Index = 0;
	for (const FLootDrop& Drop : Box->GetPendingDrops())
	{
		UItemDefinition* Def = Drop.Definition.LoadSynchronous();
		const FName ItemId = IsValid(Def) ? Def->ItemId : NAME_None;
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s x%d Lv%d (Grade=%s, Dev=%.3f)"),
			Index, *ItemId.ToString(), Drop.Count, Drop.Level,
			*Drop.GradeTag.ToString(), Drop.StatDeviation);
		++Index;
	}
}

void UGYCheatManager::GY_TakeFromLootBox(int32 DropIndex)
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	ALootBoxActor* Box = FindNearestLootBox(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Box))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_TakeFromLootBox: no loot box nearby"));
		return;
	}

	Server_TakeFromLootBox(Box, DropIndex);
}

void UGYCheatManager::GY_TakeAllLoot()
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	ALootBoxActor* Box = FindNearestLootBox(Pawn->GetWorld(), Pawn->GetActorLocation());
	if (!IsValid(Box))
	{
		UE_LOG(LogTemp, Warning, TEXT("GY_TakeAllLoot: no loot box nearby"));
		return;
	}

	Server_TakeAllLoot(Box);
}

void UGYCheatManager::Server_SpawnLootBox_Implementation(const FString& SourceId, const FString& LootTablePath)
{
	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	UDataTable* Table = LoadObject<UDataTable>(nullptr, *LootTablePath);
	if (!IsValid(Table))
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_SpawnLootBox: failed to load table %s"), *LootTablePath);
		return;
	}

	const FVector SpawnLocation = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 200.f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ALootBoxActor* Box = Pawn->GetWorld()->SpawnActor<ALootBoxActor>(
		ALootBoxActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);

	if (!IsValid(Box)) return;

	Box->LootSourceId = FName(*SourceId);
	Box->LootTable = Table;

	UE_LOG(LogTemp, Log, TEXT("Server_SpawnLootBox: spawned %s (SourceId=%s)"),
		*Box->GetName(), *SourceId);
}

void UGYCheatManager::Server_InvokeInteraction_Implementation(AActor* Target, FGameplayTag OptionTag)
{
	if (!IsValid(Target)) return;

	IInteractable* Interactable = Cast<IInteractable>(Target);
	if (Interactable == nullptr) return;

	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	Interactable->OnInteract(OptionTag, Pawn);
}

void UGYCheatManager::Server_TakeFromLootBox_Implementation(AActor* Box, int32 DropIndex)
{
	ALootBoxActor* LootBox = Cast<ALootBoxActor>(Box);
	if (!IsValid(LootBox)) return;

	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	LootBox->TakeItem(DropIndex, Pawn);
}

void UGYCheatManager::Server_TakeAllLoot_Implementation(AActor* Box)
{
	ALootBoxActor* LootBox = Cast<ALootBoxActor>(Box);
	if (!IsValid(LootBox)) return;

	APawn* Pawn = GetCheatPawn(this);
	if (!IsValid(Pawn)) return;

	LootBox->TakeAll(Pawn);
}
