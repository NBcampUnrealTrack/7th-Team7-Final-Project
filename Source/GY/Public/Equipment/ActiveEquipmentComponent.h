#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/EquipmentEntry.h"
#include "GameplayTagContainer.h"
#include "ActiveEquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UEquipmentInstance;
class UItemDefinition;

UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class GY_API UActiveEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActiveEquipmentComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UEquipmentInstance* EquipItem(const struct FInventoryEntry& Entry);

	UFUNCTION(BlueprintCallable)
	bool UnequipItem(FGameplayTag SlotTag);

	UFUNCTION(BlueprintPure)
	UEquipmentInstance* GetEquippedInstance(FGameplayTag SlotTag) const;

	void RefreshEquipment(const struct FInventoryEntry& Entry);

	void OnLoadoutSlotChanged(FGameplayTag SlotTag, FGuid NewInstanceId);

	void HandleItemEnchanted(FGuid InstanceId);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ApplyAbilitySetsFromEntry(UEquipmentInstance* Instance, const struct FInventoryEntry& Entry);
	void RevokeAbilitySets(UEquipmentInstance* Instance);

	void ApplyWeaponBaseStats(UEquipmentInstance* Instance, UItemDefinition* Def, UAbilitySystemComponent* ASC);
	void ApplyEnchantOptions(UEquipmentInstance* Instance, UItemDefinition* Def, const struct FInventoryEntry& Entry, UAbilitySystemComponent* ASC);

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Equipment")
	FEquipmentList EquippedItems;

	FDelegateHandle EnchantedHandle;
};
