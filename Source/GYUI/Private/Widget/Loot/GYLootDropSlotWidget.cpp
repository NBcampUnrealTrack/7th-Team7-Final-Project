#include "Widget/Loot/GYLootDropSlotWidget.h"

#include "CommonTextBlock.h"
#include "Loot/LootBoxActor.h"
#include "Loot/LootTypes.h"
#include "Loot/LootViewerComponent.h"
#include "Player/GYPlayerState.h"

void UGYLootDropSlotWidget::SetDrop(ALootBoxActor* InBox, int32 InDropIndex, const FLootDrop& Drop)
{
	BoundBox = InBox;
	DropIndex = InDropIndex;

	FGYItemViewData View;
	View.Definition = Drop.Definition;
	View.GradeTag = Drop.GradeTag;
	View.Level = Drop.Level;
	View.Count = Drop.Count;
	View.StatDeviation = Drop.StatDeviation;
	View.RolledOptions = Drop.RolledOptions;
	SetView(View);
}

void UGYLootDropSlotWidget::SetEmpty()
{
	BoundBox = nullptr;
	DropIndex = INDEX_NONE;
	ClearView();
}

void UGYLootDropSlotWidget::OnViewChanged(bool bIsEmpty)
{
	if (Text_Count)
	{
		if (!bIsEmpty && CurrentInfo.Count > 1)
		{
			Text_Count->SetText(FText::AsNumber(CurrentInfo.Count));
			Text_Count->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_Count->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 빈 칸이면 빈 태그 → 그래프 Switch Default → Border_Grade 숨김 (이전 등급색 리셋 포함)
	OnDropUpdated(CurrentInfo.GradeTag, CurrentInfo.Count);
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
