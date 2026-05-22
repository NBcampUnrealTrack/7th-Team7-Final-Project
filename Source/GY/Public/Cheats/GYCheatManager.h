#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GameplayTagContainer.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "GYCheatManager.generated.h"

class AActor;
class AGYEnemyCharacterBase;
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
	void GY_AddTimeShards(int32 Amount);

	UFUNCTION(Exec)
	void GY_PrintCurrency();

	UFUNCTION(Exec)
	void GY_Disassemble(int32 InvIndex);

	UFUNCTION(Exec)
	void GY_Enchant(int32 InvIndex);

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

	UFUNCTION(Exec)
	void GY_AddCameraTag(const FString& TagName);

	UFUNCTION(Exec)
	void GY_RemoveCameraTag(const FString& TagName);

	UFUNCTION(Exec)
	void GY_ToggleCameraTag(const FString& TagName);

	// Enemy Debug
	UFUNCTION(Exec)
	void GY_SpawnEnemy(const FString& EnemyTypeName);

	UFUNCTION(Exec)
	void GY_KillAllEnemies();

	UFUNCTION(Exec)
	void GY_SetEnemyBB(const FString& KeyName, bool bValue);
private:
	UFUNCTION(Server, Reliable)
	void Server_AddItem(const FString& ItemPath, int32 Count);

	UFUNCTION(Server, Reliable)
	void Server_EquipItem(const FString& ItemPath);

	UFUNCTION(Server, Reliable)
	void Server_UnequipSlot(FGameplayTag SlotTag);

	UFUNCTION(Server, Reliable)
	void Server_AddTimeShards(int32 Amount);

	UFUNCTION(Server, Reliable)
	void Server_SpawnLootBox(const FString& SourceId, const FString& LootTablePath);

	UFUNCTION(Server, Reliable)
	void Server_InvokeInteraction(AActor* Target, FGameplayTag OptionTag);

	UFUNCTION(Server, Reliable)
	void Server_TakeFromLootBox(AActor* Box, int32 DropIndex);

	UFUNCTION(Server, Reliable)
	void Server_TakeAllLoot(AActor* Box);

	UFUNCTION(Server, Reliable)
	void Server_SpawnEnemy(EEnemyType EnemyType);

	UFUNCTION(Server, Reliable)
	void Server_KillAllEnemies();
public:
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Debug")
	TMap<EEnemyType, TSoftClassPtr<AGYEnemyCharacterBase>> EnemyClassMap;

	UFUNCTION(Exec)
	void GY_TestHitCue();
};
