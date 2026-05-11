#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/EquipmentEntry.h"
#include "GameplayTagContainer.h"
#include "ActiveEquipmentComponent.generated.h"

class UEquipmentInstance;

UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class GY_API UActiveEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActiveEquipmentComponent();

	UEquipmentInstance* EquipItem(const struct FInventoryEntry& Entry);

	UFUNCTION(BlueprintCallable)
	bool UnequipItem(FGameplayTag SlotTag);

	UFUNCTION(BlueprintPure)
	UEquipmentInstance* GetEquippedInstance(FGameplayTag SlotTag) const;

	void RefreshEquipment(const struct FInventoryEntry& Entry);

	void OnLoadoutSlotChanged(FGameplayTag SlotTag, FGuid NewInstanceId);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ApplyAbilitySetsFromEntry(UEquipmentInstance* Instance, const struct FInventoryEntry& Entry);
	void RevokeAbilitySets(UEquipmentInstance* Instance);

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Equipment")
	FEquipmentList EquippedItems;
};
