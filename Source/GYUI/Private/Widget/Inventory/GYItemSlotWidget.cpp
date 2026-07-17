#include "Widget/Inventory/GYItemSlotWidget.h"

#include "CommonTextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Core/GYItemDragDropOperation.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"
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

	// 슬롯 재사용 시 드래그/히트테스트 상태를 확실히 복구 - 배열 시프트 후 상호작용 불가 방지
	bDragStarted = false;
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::Visible);

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
	bDragStarted = false;
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::Visible);
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

	// 등급 개념 없는 물약 등은 무효 태그로 강제해 BP가 테두리를 숨기게
	FGameplayTag GradeForBorder;
	if (!bIsEmpty && CurrentInfo.GradeTag.IsValid())
	{
		const UItemDefinition* Def = CurrentInfo.Definition.LoadSynchronous();
		if (IsValid(Def) && Def->CategoryTags.HasTag(GYGameplayTags::Item_Category_Equipment))
		{
			GradeForBorder = CurrentInfo.GradeTag;
		}
	}
	OnSlotUpdated(bIsEmpty, GradeForBorder, CurrentInfo.Count);
}

FGuid UGYItemSlotWidget::GetItemInstanceId()
{
	return ItemInstanceId;
}

void UGYItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
                                             UDragDropOperation*& OutOperation)
{
	if (!Container || !ItemInstanceId.IsValid()) return;

	bDragStarted = true;
	SetRenderOpacity(0.5f);

	UGYItemDragDropOperation* DragOperation = Cast<UGYItemDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UGYItemDragDropOperation::StaticClass()));

	DragOperation->FromContainer = Container;
	DragOperation->FromInstanceId = ItemInstanceId;
	DragOperation->Pivot = EDragPivot::CenterCenter;
	DragOperation->OriginSlotWidget = this;

	// 살아있는 슬롯 위젯을 드래그 레이어로 재부모화하지 않도록 별도 아이콘 위젯 사용
	if (WidgetTree)
	{
		if (UImage* DragIcon = WidgetTree->ConstructWidget<UImage>())
		{
			if (UItemDefinition* Def = CurrentInfo.Definition.LoadSynchronous())
			{
				DragIcon->SetBrushFromSoftTexture(Def->Icon, false);
			}
			const FVector2D Size = InGeometry.GetLocalSize();
			DragIcon->SetDesiredSizeOverride(Size.IsNearlyZero() ? FVector2D(64.f, 64.f) : Size);
			DragOperation->DefaultDragVisual = DragIcon;
		}
	}

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
	// 드래그 없이 좌클릭만 → 통지
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !bDragStarted && ItemInstanceId.IsValid())
	{
		// 슬롯 단위로 구독했으면 우선
		if (OnSlotClicked.IsBound())
		{
			OnSlotClicked.Broadcast(ItemInstanceId);
			return FReply::Handled();
		}
		if (UGYInventoryScreenWidget* Screen = GetTypedOuter<UGYInventoryScreenWidget>())
		{
			Screen->NotifyItemClicked(ItemInstanceId);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
