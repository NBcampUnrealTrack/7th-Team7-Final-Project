#include "Widget/Loot/GYLootDropSlotWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Items/ItemDefinition.h"
#include "Loot/LootBoxActor.h"
#include "Loot/LootTypes.h"
#include "Loot/LootViewerComponent.h"
#include "Player/GYPlayerState.h"

void UGYLootDropSlotWidget::SetDrop(ALootBoxActor* InBox, int32 InDropIndex, const FLootDrop& Drop)
{
	BoundBox = InBox;
	DropIndex = InDropIndex;

	CurrentInfo.Definition = Drop.Definition;
	CurrentInfo.GradeTag = Drop.GradeTag;
	CurrentInfo.Level = Drop.Level;
	CurrentInfo.Count = Drop.Count;
	CurrentInfo.StatDeviation = Drop.StatDeviation;
	CurrentInfo.RolledOptions = Drop.RolledOptions;

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
	CurrentInfo = FGYItemViewData();

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

FReply UGYLootDropSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
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
