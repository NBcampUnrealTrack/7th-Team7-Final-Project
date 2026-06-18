#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "GYInventoryScreenWidget.generated.h"

class IItemContainer;
class UEquipmentLoadoutComponent;
class UGYItemSlotWidget;
class UInventoryComponent;
class UPanelWidget;
struct FGYInventoryEntryMessage;
struct FGYEquipSlotMessage;
class UGYPlayerStatsWidget;

// 인벤 슬롯 좌클릭(드래그 아님) 시 발행. 호스트 화면(인첸트/루트/장착)이 바인딩해 동작 결정
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGYOnInventoryItemClicked, FGuid, InstanceId);

UCLASS(Abstract, Blueprintable)
class GYUI_API UGYInventoryScreenWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	/** 슬롯이 좌클릭 시 호출 → OnItemClicked 발행 */
	void NotifyItemClicked(const FGuid& InstanceId);

	UPROPERTY(BlueprintAssignable, Category = "GY|Inventory")
	FGYOnInventoryItemClicked OnItemClicked;

	/** 카테고리 탭에서 호출. 빈 태그면 전체 표시 */
	UFUNCTION(BlueprintCallable, Category = "GY|Inventory")
	void SetCategory(FGameplayTag CategoryTag);

	UFUNCTION(BlueprintPure, Category = "GY|Inventory")
	FGameplayTag GetCategory() const { return CurrentCategory; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Inventory")
	TSubclassOf<UGYItemSlotWidget> SlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Inventory", meta = (ClampMin = 1))
	int32 GridSlotCount = 48;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Inventory", meta = (Categories = "Item.Category"))
	FGameplayTag InitialCategory;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotContainer;

	// 인벤토리 우측에 표시하는 캐릭터 스탯 패널
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UGYPlayerStatsWidget> StatsPanel;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Inventory")
	void OnCategoryChanged(FGameplayTag NewCategory);

private:
	void EnsureSlots();
	void Refresh();
	UInventoryComponent* ResolveInventory() const;
	UEquipmentLoadoutComponent* ResolveLoadout() const;
	void HandleEntryChanged(FGameplayTag Channel, const FGYInventoryEntryMessage& Msg);
	// 장착/해제 시 인벤 그리드 갱신 (장착 중인 아이템은 가방에서 숨김)
	void HandleLoadoutChanged(FGameplayTag Channel, const FGYEquipSlotMessage& Msg);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGYItemSlotWidget>> SlotWidgets;

	UPROPERTY()
	TScriptInterface<IItemContainer> Container;

	FGameplayTag CurrentCategory;
	FGameplayMessageListenerHandle ListenerHandle;
	FGameplayMessageListenerHandle LoadoutListenerHandle;
};
