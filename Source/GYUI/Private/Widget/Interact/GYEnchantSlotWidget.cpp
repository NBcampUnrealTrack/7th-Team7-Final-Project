#include "Widget/Interact/GYEnchantSlotWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Core/GYItemDragDropOperation.h"
#include "Core/GameplayTags/EventTags.h"
#include "Items/ItemContainer.h"
#include "Player/GYPlayerState.h"
#include "Widget/Interact/GYEnchantWidget.h"

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

	// 대상 지정은 인첸트 위젯이 중앙 처리 (슬롯 표시 + 전용 옵션 패널)
	if (UGYEnchantWidget* EnchantWidget = GetTypedOuter<UGYEnchantWidget>())
	{
		EnchantWidget->SetTarget(DragOperation->FromInstanceId);
	}
	return true;
}
