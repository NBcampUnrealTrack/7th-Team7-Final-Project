#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GameplayTagContainer.h"
#include "GYCheatManager.generated.h"

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

private:
	UFUNCTION(Server, Reliable)
	void Server_AddItem(const FString& ItemPath, int32 Count);

	UFUNCTION(Server, Reliable)
	void Server_EquipItem(const FString& ItemPath);

	UFUNCTION(Server, Reliable)
	void Server_UnequipSlot(FGameplayTag SlotTag);
};
