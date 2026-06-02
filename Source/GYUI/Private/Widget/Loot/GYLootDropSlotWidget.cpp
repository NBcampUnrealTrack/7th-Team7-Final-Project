#include "Widget/Loot/GYLootDropSlotWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Items/ItemDefinition.h"
#include "Loot/LootBoxActor.h"
#include "Loot/LootTypes.h"
#include "Loot/LootViewerComponent.h"
#include "Player/GYPlayerState.h"

void UGYLootDropSlotWidget::SetDrop(ALootBoxActor* InBox, int32 InDropIndex, const FLootDrop& Drop)
{
	BoundBox = InBox;
	DropIndex = InDropIndex;

	UItemDefinition* Def = Drop.Definition.LoadSynchronous();
	if (Image_Icon)
	{
		if (IsValid(Def))
		{
			Image_Icon->SetOpacity(1.f);
			Image_Icon->SetBrushFromSoftTexture(Def->Icon, false);
		}
		else
		{
			Image_Icon->SetOpacity(0.f);
		}
	}

	if (Text_Count)
	{
		if (Drop.Count > 1)
		{
			Text_Count->SetText(FText::AsNumber(Drop.Count));
			Text_Count->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_Count->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	OnDropUpdated(Drop.GradeTag, Drop.Count);
}

void UGYLootDropSlotWidget::SetEmpty()
{
	BoundBox = nullptr;
	DropIndex = INDEX_NONE;

	if (Image_Icon)
	{
		Image_Icon->SetOpacity(0.f);
	}

	if (Text_Count)
	{
		Text_Count->SetVisibility(ESlateVisibility::Hidden);
	}

	// 빈 태그 → 그래프 Switch Default → Border_Grade 숨김 (이전 등급색 리셋 포함)
	OnDropUpdated(FGameplayTag(), 0);
}

void UGYLootDropSlotWidget::RequestTake()
{
	if (!BoundBox.IsValid()) return;
	if (DropIndex == INDEX_NONE) return;

	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	if (!IsValid(PS)) return;

	ULootViewerComponent* Viewer = PS->GetLootViewerComponent();
	if (!IsValid(Viewer)) return;

	Viewer->Server_TakeLootItem(BoundBox.Get(), DropIndex);
}
