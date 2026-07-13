#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/ItemEnums.h"
#include "GameplayTagContainer.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemContainer.h"
#include "Items/Fragments/ItemFragment_Consumable.h"
#include "Persistence/GYSaveable.h"
#include "Persistence/GYSaveSectionKeys.h"
#include "Templates/Function.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged,
	const FGuid& /*InstanceId*/,
	EInventoryEventType /*EventType*/);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class GY_API UInventoryComponent : public UActorComponent, public IItemContainer, public IGYSaveable
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	virtual void BeginPlay() override;

	virtual int32 GetCapacity() const override;
	virtual int32 TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId) override;

	// 새 엔트리를 Added 알림 전에 완성시키는 획득 경로 — 구독자가 미완성 아이템을 보지 않는다.
	// InitNewEntry 는 새로 생성되는 엔트리에만 적용 (기존 스택 병합분은 제외)
	int32 TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId, TFunctionRef<void(FInventoryEntry&)> InitNewEntry);

	// 현재 가방이 차지한 슬롯 수 (장착 중인 아이템은 제외 — 로드아웃이 참조만 하므로 용량에서 빠짐)
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetOccupiedSlotCount() const;
	virtual bool TryRemoveItem(const FGuid& InstanceId, int32 Count) override;
	virtual bool MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator) override;
	virtual const FInventoryEntry* FindEntry(const FGuid& InstanceId) const override;
	virtual const TArray<FInventoryEntry>& GetEntries() const override { return Inventory.Entries; }
	virtual void NotifyContainerChanged(const FGuid& InstanceId, EInventoryEventType EventType) override;
	virtual bool InsertEntry(const FInventoryEntry& Entry) override;
	virtual bool TakeEntry(const FGuid& InstanceId, FInventoryEntry& OutEntry) override;
	virtual FGameplayTag GetContainerTag() const override;

	TArray<FInventoryEntry> GetAllEntriesByCategory(FGameplayTag CategoryTag) const;


	UFUNCTION(Server, Reliable)
	void Server_RequestEnchant(const FGuid& InstanceId);

	// [SERVER→OWNER CLIENT] 인챈트 성공 시에만 사운드 재생
	UFUNCTION(Client, Reliable)
	void Client_PlayEnchantSound(FGameplayTag SoundTag);

	FOnInventoryChanged OnInventoryChanged;

	// IGYSaveable
	virtual FString GetSaveSectionKey() const override { return GYSaveSectionKeys::Inventory; }
	virtual TSharedPtr<FJsonValue> ExportSaveData() const override;
	virtual void ImportSaveData(const TSharedPtr<FJsonValue>& Data) override;

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

