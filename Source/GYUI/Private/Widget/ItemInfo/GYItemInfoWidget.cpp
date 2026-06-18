#include "Widget/ItemInfo/GYItemInfoWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Enchant/EnchantOptionResolver.h"
#include "Engine/DataTable.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/EnchantOptionRow.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"
#include "Widget/ItemInfo/EnchantMagnitudeDisplayRow.h"
#include "Enchant/RolledEnchantOption.h"

EGYItemInfoTrigger UGYItemInfoWidget::GetEffectiveTrigger() const
{
	return bPinned ? EGYItemInfoTrigger::Direct : Trigger;
}

void UGYItemInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 첫 우클릭 전엔 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayMessageSubsystem& Messaging = UGameplayMessageSubsystem::Get(World);

	switch (GetEffectiveTrigger()) // 모드별 채널 구독
 	{
	case EGYItemInfoTrigger::Hover:
		TriggerListenerHandle = Messaging.RegisterListener(
			GYGameplayTags::Message_UI_ShowItemInfo, this, &UGYItemInfoWidget::HandleHoverItemInfo);
		break;
	case EGYItemInfoTrigger::Pin:
		TriggerListenerHandle = Messaging.RegisterListener(
			GYGameplayTags::Message_UI_PinItemInfo, this, &UGYItemInfoWidget::HandlePinItemInfo);
		break;
	case EGYItemInfoTrigger::Direct:
		break;
	}
	// 리롤 등 표시 중 아이템 변경 갱신은 공용
	EntryListenerHandle = Messaging.RegisterListener(
		GYGameplayTags::Message_Inventory_EntryChanged, this, &UGYItemInfoWidget::HandleEntryChanged);
}

void UGYItemInfoWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& Messaging = UGameplayMessageSubsystem::Get(World);
		if (TriggerListenerHandle.IsValid()) Messaging.UnregisterListener(TriggerListenerHandle);
		if (EntryListenerHandle.IsValid()) Messaging.UnregisterListener(EntryListenerHandle);
	}
	TriggerListenerHandle = FGameplayMessageListenerHandle();
	EntryListenerHandle = FGameplayMessageListenerHandle();

	Super::NativeDestruct();
}

void UGYItemInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (GetEffectiveTrigger() != EGYItemInfoTrigger::Hover) return;
	if (!bFollowMouse) return;
	if (GetVisibility() == ESlateVisibility::Collapsed) return;
	UpdatePositionToMouse();
}

void UGYItemInfoWidget::ShowItem(const FGYItemViewData& Item)
{
	if (Item.Definition.IsNull())
	{
		Hide();
		return;
	}
	ApplyView(Item);
}

void UGYItemInfoWidget::HandleHoverItemInfo(FGameplayTag, const FGYItemViewData& Item)
{
	if (Item.Definition.IsNull())
	{
		if (CurrentSource.Get() == Item.Source.Get() || !CurrentSource.IsValid())
		{
			Hide();
		}
		return;
	}
	ApplyView(Item);
}

void UGYItemInfoWidget::HandlePinItemInfo(FGameplayTag, const FGYItemViewData& Item)
{
	if (Item.Definition.IsNull()) return;

	const bool bShown = GetVisibility() != ESlateVisibility::Collapsed;
	if (bShown && CurrentSource.Get() == Item.Source.Get() && CurrentInstanceId == Item.InstanceId)
	{
		Hide();
		return;
	}

	ApplyView(Item);
}

void UGYItemInfoWidget::HandleEntryChanged(FGameplayTag, const FGYInventoryEntryMessage& Msg)
{
	// 표시 중이고, 변경된 아이템이 지금 보여주는 그 아이템일 때만 갱신
	if (GetVisibility() == ESlateVisibility::Collapsed) return;
	if (!CurrentInstanceId.IsValid() || Msg.InstanceId != CurrentInstanceId) return;

	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UInventoryComponent* Inventory = IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
	const FInventoryEntry* Entry = IsValid(Inventory) ? Inventory->FindEntry(CurrentInstanceId) : nullptr;

	// 아이템이 사라졌으면 패널 닫음
	if (Entry == nullptr)
	{
		Hide(); return;
	}

	FGYItemViewData View;
	View.Definition = Entry->Definition;
	View.InstanceId = Entry->InstanceId;
	View.GradeTag = Entry->GradeTag;
	View.Level = Entry->Level;
	View.Count = Entry->StackCount;
	View.StatDeviation = Entry->StatDeviation;
	View.RolledOptions = Entry->RolledOptions;
	View.Source = CurrentSource;
	ApplyView(View);
}

void UGYItemInfoWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
	CurrentSource = nullptr;
	CurrentInstanceId = FGuid();
}

void UGYItemInfoWidget::ApplyView(const FGYItemViewData& Item)
{
	UItemDefinition* Def = Item.Definition.LoadSynchronous();
	if (!IsValid(Def)) return;

	CurrentSource = Item.Source;
	CurrentInstanceId = Item.InstanceId;
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (Image_Icon)
	{
		Image_Icon->SetBrushFromSoftTexture(Def->Icon, false);
	}

	if (Text_Name)
	{
		Text_Name->SetText(Def->DisplayName);
	}

	if (Text_Name)
	{
		Text_Name->SetText(Def->DisplayName);
	}

	if (Text_Description)
	{
		Text_Description->SetText(Def->Description);
		Text_Description->SetVisibility(Def->Description.IsEmpty() ? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	if (Text_Level)
	{
		Text_Level->SetText(FText::AsNumber(Item.Level));
	}

	if (Text_Grade)
	{
		// 등급 태그 마지막 세그먼트를 표시 (예: Item.Grade.Legendary → Legendary)
		FString TagString = Item.GradeTag.IsValid() ? Item.GradeTag.GetTagName().ToString() : FString();
		FString GradeName;
		if (!TagString.IsEmpty() && !TagString.Split(TEXT("."), nullptr, &GradeName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			GradeName = TagString;
		}
		Text_Grade->SetText(FText::FromString(GradeName));
	}

	ApplyGradeBorder(Item.GradeTag);

	if (Text_EnchantOptions)
	{
		UDataTable* DisplayTable = MagnitudeDisplayTable.LoadSynchronous();

		TArray<FText> Lines;
		for (const FRolledEnchantOption& Option : Item.RolledOptions)
		{
			const FEnchantOptionRow* Row = EnchantOptionResolver::FindRow(Def, Option.OptionId);
			const FText Label = (Row != nullptr && !Row->DisplayName.IsEmpty()) ? Row->DisplayName : FText::FromName(Option.OptionId);

			TArray<FText> Phrases;
			for (const FRolledMagnitude& Magnitude : Option.Magnitudes)
			{
				Phrases.Add(FormatMagnitude(Magnitude, DisplayTable));
			}

			if (Phrases.IsEmpty())
			{
				Lines.Add(Label);
				continue;
			}

			const FText Joined = FText::Join(INVTEXT(", "), Phrases);
			Lines.Add(Label.IsEmpty() ? Joined : FText::Format(INVTEXT("{0}:   {1}"), Label, Joined));
		}

		Text_EnchantOptions->SetText(FText::Join(INVTEXT("\n"), Lines));
		Text_EnchantOptions->SetVisibility(Lines.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 호버, bFollowMouse일 때 즉시 마우스 위치로
	if (GetEffectiveTrigger() == EGYItemInfoTrigger::Hover && bFollowMouse)
	{
		UpdatePositionToMouse();
	}
	OnItemInfoUpdated(Item.GradeTag);
}

void UGYItemInfoWidget::ApplyGradeBorder(FGameplayTag GradeTag)
{
	const FLinearColor Color = GetBorderColorForGrade(GradeTag);
	if (Border_Grade) Border_Grade->SetBrushColor(Color);
	if (Image_GradeBorder) Image_GradeBorder->SetColorAndOpacity(Color);
}

FLinearColor UGYItemInfoWidget::GetBorderColorForGrade(FGameplayTag GradeTag) const
{
	if (GradeTag.IsValid())
	{
		if (const FLinearColor* Exact = GradeBorderColors.Find(GradeTag)) return *Exact;
		for (const TPair<FGameplayTag, FLinearColor>& Pair : GradeBorderColors)
		{
			if (GradeTag.MatchesTag(Pair.Key)) return Pair.Value;
		}
	}
	return DefaultBorderColor;
}

void UGYItemInfoWidget::UpdatePositionToMouse()
{
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
	if (CanvasSlot == nullptr) return;
	APlayerController* PC = GetOwningPlayer();

	if (!IsValid(PC)) return;
	const FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(PC);
	CanvasSlot->SetPosition(MousePos + MouseOffset);
}

FText UGYItemInfoWidget::FormatMagnitude(const FRolledMagnitude& Magnitude, UDataTable* DisplayTable) const
{
	const FEnchantMagnitudeDisplayRow* Display = IsValid(DisplayTable)
		? DisplayTable->FindRow<FEnchantMagnitudeDisplayRow>(Magnitude.MagnitudeTag.GetTagName(), TEXT("FormatMagnitude"), false)
		: nullptr;

	// 롤값은 이미 매그니튜드 자리수로 양자화됨 → 불필요한 0만 떼고 그대로 표시
	FNumberFormattingOptions NumberOptions;
	NumberOptions.MinimumFractionalDigits = 0;
	NumberOptions.MaximumFractionalDigits = 3;
	const FText ValueText = FText::AsNumber(Magnitude.Value, &NumberOptions);

	if (Display != nullptr && !Display->Format.IsEmpty())
	{
		return FText::Format(Display->Format, ValueText);
	}

	// 매핑 없으면 태그 마지막 세그먼트 + 값
	FString TagString = Magnitude.MagnitudeTag.GetTagName().ToString();
	FString TagLeaf;
	if (TagString.IsEmpty() || !TagString.Split(TEXT("."), nullptr, &TagLeaf, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		TagLeaf = TagString;
	}
	return FText::Format(INVTEXT("{0} {1}"), FText::FromString(TagLeaf), ValueText);
}
