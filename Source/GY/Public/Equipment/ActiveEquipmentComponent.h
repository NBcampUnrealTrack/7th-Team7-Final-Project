#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/EquipmentEntry.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "ActiveEquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UEquipmentInstance;
class UEquipmentLoadoutComponent;
class UGameplayEffect;
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

	void RemoveAllVisuals();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRemoveAllVisuals();

	void ReapplyAnimLayers();

	void RefreshEquipment(const struct FInventoryEntry& Entry);

	void OnLoadoutSlotChanged(FGameplayTag SlotTag, FGuid NewInstanceId);

	// [SERVER] owner 폰의 PlayerState Loadout 변경을 구독하고 현재 로드아웃으로 초기 동기화한다.
	void InitializeLoadoutBinding();

	void HandleItemEnchanted(FGuid InstanceId);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ApplyAbilitySetsFromEntry(UEquipmentInstance* Instance, const struct FInventoryEntry& Entry);
	void RevokeAbilitySets(UEquipmentInstance* Instance);

	void ApplyWeaponBaseStats(UEquipmentInstance* Instance, UItemDefinition* Def, UAbilitySystemComponent* ASC);
	void ApplyArmorBaseStats(UEquipmentInstance* Instance, UItemDefinition* Def, UAbilitySystemComponent* ASC);
	void ApplyEnchantOptions(UEquipmentInstance* Instance, const struct FInventoryEntry& Entry, UAbilitySystemComponent* ASC);

	void ApplyFlatStatEffect(UEquipmentInstance* Instance, UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> EffectClass, float Value);

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Equipment")
	FEquipmentList EquippedItems;

	FDelegateHandle EnchantedHandle;

	// 구독한 Loadout(PlayerState 소유). EndPlay에서 구독 해제용.
	TWeakObjectPtr<UEquipmentLoadoutComponent> BoundLoadout;
};
