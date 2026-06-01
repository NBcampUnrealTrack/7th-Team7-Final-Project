#include "Widget/Inventory/GYItemSlotWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"

void UGYItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetEmpty();
}

void UGYItemSlotWidget::SetEntry(const FInventoryEntry& Entry)
{
	UItemDefinition* Def = Entry.Definition.LoadSynchronous();
	if (!IsValid(Def))
	{
		SetEmpty();
		return;
	}

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
