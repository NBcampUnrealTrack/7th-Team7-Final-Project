#include "Widget/Interact/GYEnchantSlotWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Core/GYItemDragDropOperation.h"
#include "Core/GameplayTags/EventTags.h"
#include "Inventory/GA_TransferItem.h"
#include "Items/ItemContainer.h"
#include "Player/GYPlayerState.h"

void UGYEnchantSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetEmpty();
}

void UGYEnchantSlotWidget::NativeDestruct()
{
	Super::NativeDestruct();
	SetEmpty();
}

FReply UGYEnchantSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Handled();
}


bool UGYEnchantSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UGYItemDragDropOperation* DragOperation = Cast<UGYItemDragDropOperation>(InOperation);
	if (!DragOperation || !DragOperation->FromContainer) return false;

	if (DragOperation->OriginSlotWidget.IsValid())
	{
		DragOperation->OriginSlotWidget->SetRenderOpacity(1.0f);
	}

	SetEntry(*DragOperation->FromContainer->FindEntry(DragOperation->FromInstanceId));
	return true;
}
