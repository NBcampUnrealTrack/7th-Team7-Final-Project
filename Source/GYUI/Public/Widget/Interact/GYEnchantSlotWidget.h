#pragma once

#include "CoreMinimal.h"
#include "Widget/Inventory/GYItemSlotWidget.h"
#include "GYEnchantSlotWidget.generated.h"

/**
 *
 */
UCLASS()
class GYUI_API UGYEnchantSlotWidget : public UGYItemSlotWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};
