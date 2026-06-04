#include "Widget/Inventory/GYItemSlotWidget.h"

#include "CommonTextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Core/GYItemDragDropOperation.h"
#include "Inventory/InventoryEntry.h"
#include "Widget/Inventory/GYInventoryScreenWidget.h"

void UGYItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetEmpty();
}

void UGYItemSlotWidget::SetContainer(TScriptInterface<IItemContainer> InContainer)
{
	Container = InContainer;
}

void UGYItemSlotWidget::SetEntry(const FInventoryEntry& Entry)
{
	if (Entry.Definition.IsNull())
	{
		SetEmpty();
		return;
	}

	SetRenderOpacity(1.0f);
	ItemInstanceId = Entry.InstanceId;

	FGYItemViewData View;
	View.Definition = Entry.Definition;
	View.InstanceId = Entry.InstanceId;
	View.GradeTag = Entry.GradeTag;
	View.Level = Entry.Level;
	View.Count = Entry.StackCount;
	View.StatDeviation = Entry.StatDeviation;
	View.RolledOptions = Entry.RolledOptions;
	SetView(View);
}

void UGYItemSlotWidget::SetEmpty()
{
	SetRenderOpacity(1.0f);
	ItemInstanceId = FGuid();
	ClearView();
}

void UGYItemSlotWidget::OnViewChanged(bool bIsEmpty)
{
	if (Text_StackCount)
	{
		if (!bIsEmpty && CurrentInfo.Count > 1)
		{
			Text_StackCount->SetText(FText::AsNumber(CurrentInfo.Count));
			Text_StackCount->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_StackCount->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	OnSlotUpdated(bIsEmpty, CurrentInfo.GradeTag, CurrentInfo.Count);
}

FGuid UGYItemSlotWidget::GetItemInstanceId()
{
	return ItemInstanceId;
}

void UGYItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
                                             UDragDropOperation*& OutOperation)
{
	if (!Container) return;

	SetRenderOpacity(0.5f);
	bDragStarted = true;

	UGYItemDragDropOperation* DragOperation = Cast<UGYItemDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UGYItemDragDropOperation::StaticClass()));

	DragOperation->FromContainer = Container;
	DragOperation->FromInstanceId = ItemInstanceId;
	DragOperation->Pivot = EDragPivot::MouseDown;
	DragOperation->OriginSlotWidget = this;
	DragOperation->DefaultDragVisual = this;

	OutOperation = DragOperation;
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
}

void UGYItemSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
	SetRenderOpacity(1.0f);
}

FReply UGYItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		bDragStarted = false;
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	// 우클릭 정보 패널은 베이스가 처리
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGYItemSlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 드래그 없이 좌클릭만 → 화면에 통지 (호스트가 장착/대상지정/되돌리기 등 결정)
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !bDragStarted && ItemInstanceId.IsValid())
	{
		if (UGYInventoryScreenWidget* Screen = GetTypedOuter<UGYInventoryScreenWidget>())
		{
			Screen->NotifyItemClicked(ItemInstanceId);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
