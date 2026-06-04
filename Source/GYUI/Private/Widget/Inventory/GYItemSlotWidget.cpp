#include "Widget/Inventory/GYItemSlotWidget.h"

#include "CommonTextBlock.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Core/GYItemDragDropOperation.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/GA_TransferItem.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"

void UGYItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
                                             UDragDropOperation*& OutOperation)
{
	if (!Container) return;

	SetRenderOpacity(0.5f);

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
	UItemDefinition* Def = Entry.Definition.LoadSynchronous();
	if (!IsValid(Def))
	{
		SetEmpty();
		return;
	}

	SetRenderOpacity(1.0f);
	ItemInstanceId = Entry.InstanceId;

	CurrentInfo.Definition = Entry.Definition;
	CurrentInfo.GradeTag = Entry.GradeTag;
	CurrentInfo.Level = Entry.Level;
	CurrentInfo.Count = Entry.StackCount;
	CurrentInfo.StatDeviation = Entry.StatDeviation;
	CurrentInfo.RolledOptions = Entry.RolledOptions;

	if (Image_Icon)
	{
		Image_Icon->SetOpacity(1.f);
		Image_Icon->SetBrushFromSoftTexture(Def->Icon, false);

	}

	if (Text_StackCount)
	{
		if (Entry.StackCount > 1)
		{
			Text_StackCount->SetText(FText::AsNumber(Entry.StackCount));
			Text_StackCount->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_StackCount->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	OnSlotUpdated(false, Entry.GradeTag, Entry.StackCount);
}

void UGYItemSlotWidget::SetEmpty()
{
	SetRenderOpacity(1.0f);
	ItemInstanceId = FGuid();
	CurrentInfo = FGYItemViewData();

	if (Image_Icon)
	{
		Image_Icon->SetBrushFromTexture(nullptr);
		Image_Icon->SetOpacity(0.f);
	}

	if (Text_StackCount)
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Hidden);
	}

	OnSlotUpdated(true, FGameplayTag::EmptyTag, 0);
}

FGuid UGYItemSlotWidget::GetItemInstanceId()
{
	return ItemInstanceId;
}

FReply UGYItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	// 우클릭 → 아이템 정보 패널 표시 (빈 칸이면 무시)
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && !CurrentInfo.Definition.IsNull())
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_UI_ShowItemInfo, CurrentInfo);
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

}
