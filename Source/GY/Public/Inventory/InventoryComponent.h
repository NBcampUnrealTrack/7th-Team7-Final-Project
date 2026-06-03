#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/ItemEnums.h"
#include "GameplayTagContainer.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemContainer.h"
#include "Templates/Function.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged,
	const FGuid& /*InstanceId*/,
	EInventoryEventType /*EventType*/);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class GY_API UInventoryComponent : public UActorComponent, public IItemContainer
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	virtual void BeginPlay() override;

	virtual int32 GetCapacity() const override;
	virtual bool TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId) override;
	virtual bool TryRemoveItem(const FGuid& InstanceId, int32 Count) override;
	virtual bool MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator) override;
	virtual const FInventoryEntry* FindEntry(const FGuid& InstanceId) const override;
	virtual const TArray<FInventoryEntry>& GetEntries() const override { return Inventory.Entries; }
	virtual void NotifyContainerChanged(const FGuid& InstanceId, EInventoryEventType EventType) override;
	virtual bool InsertEntry(const FInventoryEntry& Entry) override;

	TArray<FInventoryEntry> GetAllEntriesByCategory(FGameplayTag CategoryTag) const;


	UFUNCTION(Server, Reliable)
	void Server_RequestEnchant(const FGuid& InstanceId);

	FOnInventoryChanged OnInventoryChanged;

	/** 인벤 변동 단일 진입점 — 델리게이트 발화 + 포션 슬롯 GMS publish */

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Inventory")
	FInventoryList Inventory;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Inventory")
	int32 Capacity;
private:
	/** 인벤 전체 스캔 후 ChargePool 별 스택 집계 → GMS publish. 빈슬롯 전환 위해 이전 publish 셋 캐시 */
	void BroadcastPotionSnapshots();



private:
	TSet<FGameplayTag> LastPublishedPotionTags;
};

