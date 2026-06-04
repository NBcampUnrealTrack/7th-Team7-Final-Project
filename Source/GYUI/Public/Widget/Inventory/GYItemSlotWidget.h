#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYItemSlotWidget.generated.h"

class IItemContainer;
class UImage;
class UCommonTextBlock;
struct FInventoryEntry;

UCLASS(Abstract, Blueprintable)
class GYUI_API UGYItemSlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetContainer(TScriptInterface<IItemContainer> InContainer);
	void SetEntry(const FInventoryEntry& Entry);
	void SetEmpty();
	FGuid GetItemInstanceId();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_StackCount;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Inventory")
	void OnSlotUpdated(bool bIsEmpty, FGameplayTag GradeTag, int32 StackCount);

	UPROPERTY()
	TScriptInterface<IItemContainer> Container;
	FGuid ItemInstanceId;
};
