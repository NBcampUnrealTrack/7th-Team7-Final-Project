#pragma once

#include "CoreMinimal.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "GameFramework/Actor.h"
#include "GYServerCheatProxy.generated.h"


class AGYEnemyCharacterBase;

UCLASS()
class GY_API AGYServerCheatProxy : public AActor
{
	GENERATED_BODY()

public:
	AGYServerCheatProxy();

	UFUNCTION(Server, Reliable)
	void Server_AddItem(const FString& ItemPath, int32 Count);

	UFUNCTION(Server, Reliable)
	void Server_EquipItem(const FString& ItemPath);

	UFUNCTION(Server, Reliable)
	void Server_UnequipSlot(FGameplayTag SlotTag);

	UFUNCTION(Server, Reliable)
	void Server_AddTimeShards(int32 Amount);

	UFUNCTION(Server, Reliable)
	void Server_SpawnLootBox(const FString& RegionDataPath);

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

	UFUNCTION(Server, Reliable)
	void Server_AddXP(float Amount);

	UFUNCTION(Server, Reliable)
	void Server_Suicide();

	UFUNCTION(Server, Reliable)
	void Server_Invulnerable_Toggle();

	UFUNCTION(Server, Reliable)
	void Server_Increase_Stagger(float Amount);

public:
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Debug")
	TMap<EEnemyType, TSoftClassPtr<AGYEnemyCharacterBase>> EnemyClassMap;

	TWeakObjectPtr<APlayerController> OwnerController;


};
