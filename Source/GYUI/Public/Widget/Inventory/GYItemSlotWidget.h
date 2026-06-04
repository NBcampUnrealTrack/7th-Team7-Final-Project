#pragma once

#include "CoreMinimal.h"
#include "Widget/Slot/GYItemSlotBase.h"
#include "GameplayTagContainer.h"
#include "GYItemSlotWidget.generated.h"

class IItemContainer;
class UCommonTextBlock;
struct FInventoryEntry;

UCLASS(Abstract, Blueprintable)
class GYUI_API UGYItemSlotWidget : public UGYItemSlotBase
{
	GENERATED_BODY()

public:
	void SetContainer(TScriptInterface<IItemContainer> InContainer);
	void SetEntry(const FInventoryEntry& Entry);
	void SetEmpty();
	FGuid GetItemInstanceId();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeConstruct() override;
	virtual void OnViewChanged(bool bIsEmpty) override;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_StackCount;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Inventory")
	void OnSlotUpdated(bool bIsEmpty, FGameplayTag GradeTag, int32 StackCount);

	UPROPERTY()
	TScriptInterface<IItemContainer> Container;
	FGuid ItemInstanceId;

	// 드래그가 시작됐는지 — 좌클릭(클릭만)과 드래그 구분용
	bool bDragStarted = false;
};
