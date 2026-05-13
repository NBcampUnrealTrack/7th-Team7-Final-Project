#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GameplayTagContainer.h"
#include "GYCheatManager.generated.h"

class AActor;

UCLASS()
class GY_API UGYCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void GY_AddItem(const FString& ItemPath, int32 Count = 1);

	UFUNCTION(Exec)
	void GY_EquipItem(const FString& ItemPath);

	UFUNCTION(Exec)
	void GY_UnequipSlot(const FString& SlotTagName);

	UFUNCTION(Exec)
	void GY_PrintInventory();

	UFUNCTION(Exec)
	void GY_PrintLoadout();

	UFUNCTION(Exec)
	void GY_SpawnLootBox(const FString& SourceId, const FString& LootTablePath);

	UFUNCTION(Exec)
	void GY_GetNearestInteractionOptions();

	UFUNCTION(Exec)
	void GY_InvokeInteraction(const FString& OptionTagName);

	UFUNCTION(Exec)
	void GY_PrintLootBoxContents();

	UFUNCTION(Exec)
	void GY_TakeFromLootBox(int32 DropIndex);

	UFUNCTION(Exec)
	void GY_TakeAllLoot();

private:
	UFUNCTION(Server, Reliable)
	void Server_AddItem(const FString& ItemPath, int32 Count);

	UFUNCTION(Server, Reliable)
	void Server_EquipItem(const FString& ItemPath);

	UFUNCTION(Server, Reliable)
	void Server_UnequipSlot(FGameplayTag SlotTag);

	UFUNCTION(Server, Reliable)
	void Server_SpawnLootBox(const FString& SourceId, const FString& LootTablePath);

	UFUNCTION(Server, Reliable)
	void Server_InvokeInteraction(AActor* Target, FGameplayTag OptionTag);

	UFUNCTION(Server, Reliable)
	void Server_TakeFromLootBox(AActor* Box, int32 DropIndex);

	UFUNCTION(Server, Reliable)
	void Server_TakeAllLoot(AActor* Box);
};
