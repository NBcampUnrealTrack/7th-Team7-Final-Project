#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/ItemEnums.h"
#include "GameplayTagContainer.h"
#include "Inventory/InventoryEntry.h"
#include "Templates/Function.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged,
	const FGuid& /*InstanceId*/,
	EInventoryEventType /*EventType*/);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class GY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	bool TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId);
	bool TryRemoveItem(const FGuid& InstanceId, int32 Count);
	bool MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator);
	const FInventoryEntry* FindEntry(const FGuid& InstanceId) const;
	TArray<FInventoryEntry> GetAllEntriesByCategory(FGameplayTag CategoryTag) const;

	const TArray<FInventoryEntry>& GetEntries() const { return Inventory.Entries; }

	UFUNCTION(Server, Reliable)
	void Server_RequestEnchant(const FGuid& InstanceId);

	UFUNCTION(Server, Reliable)
	void Server_RequestDisassemble(const FGuid& InstanceId);

	FOnInventoryChanged OnInventoryChanged;

	/** 인벤 변동 단일 진입점 — 델리게이트 발화 + 포션 슬롯 GMS publish */
	void NotifyInventoryChanged(const FGuid& InstanceId, EInventoryEventType EventType);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Inventory")
	FInventoryList Inventory;

private:
	/** 인벤 전체 스캔 후 ChargePool 별 스택 집계 → GMS publish. 빈슬롯 전환 위해 이전 publish 셋 캐시 */
	void BroadcastPotionSnapshots();

	TSet<FGameplayTag> LastPublishedPotionTags;
};
