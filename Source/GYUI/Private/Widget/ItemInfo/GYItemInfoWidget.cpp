#include "Widget/ItemInfo/GYItemInfoWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Enchant/EnchantOptionResolver.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/EnchantOptionRow.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"
#include "Widget/ItemInfo/EnchantMagnitudeDisplayRow.h"
#include "Enchant/RolledEnchantOption.h"
#include "Engine/DataTable.h"

void UGYItemInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 첫 우클릭 전엔 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& Messaging = UGameplayMessageSubsystem::Get(World);
		// pinned 패널은 우클릭 브라우징 메시지를 듣지 않음 (대상만 직접 표시)
		if (!bPinned)
		{
			ListenerHandle = Messaging.RegisterListener(
				GYGameplayTags::Message_UI_ShowItemInfo, this, &UGYItemInfoWidget::HandleShowItemInfo);
		}
		// 리롤 등 표시 중 아이템 변경 갱신은 공용
		EntryListenerHandle = Messaging.RegisterListener(
			GYGameplayTags::Message_Inventory_EntryChanged, this, &UGYItemInfoWidget::HandleEntryChanged);
	}
}

void UGYItemInfoWidget::ShowItem(const FGYItemViewData& Item)
{
	if (Item.Definition.IsNull())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		CurrentSource = nullptr;
		CurrentInstanceId = FGuid();
		return;
	}
	ApplyView(Item);
}

void UGYItemInfoWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& Messaging = UGameplayMessageSubsystem::Get(World);
		if (ListenerHandle.IsValid()) Messaging.UnregisterListener(ListenerHandle);
		if (EntryListenerHandle.IsValid()) Messaging.UnregisterListener(EntryListenerHandle);
	}
	ListenerHandle = FGameplayMessageListenerHandle();
	EntryListenerHandle = FGameplayMessageListenerHandle();

	Super::NativeDestruct();
}

void UGYItemInfoWidget::HandleShowItemInfo(FGameplayTag, const FGYItemViewData& Item)
{
	// 같은 슬롯 + 같은 아이템을 다시 우클릭 + 이미 표시 중이면 토글로 닫음
	// (대상 교체처럼 Source는 같지만 아이템이 다른 경우는 토글 아님 → 갱신)
	const bool bShown = GetVisibility() != ESlateVisibility::Collapsed;
	if (bShown && CurrentSource.Get() == Item.Source.Get() && CurrentInstanceId == Item.InstanceId)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		CurrentSource = nullptr;
		CurrentInstanceId = FGuid();
		return;
	}

	if (Item.Definition.IsNull())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		CurrentSource = nullptr;
		CurrentInstanceId = FGuid();
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
		SetVisibility(ESlateVisibility::Collapsed);
		CurrentSource = nullptr;
		CurrentInstanceId = FGuid();
		return;
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

void UGYItemInfoWidget::ApplyView(const FGYItemViewData& Item)
{
	UItemDefinition* Def = Item.Definition.LoadSynchronous();
	if (!IsValid(Def)) return;

	CurrentSource = Item.Source;
	CurrentInstanceId = Item.InstanceId;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (Image_Icon)
	{
		Image_Icon->SetBrushFromSoftTexture(Def->Icon, false);
	}

	if (Text_Name)
	{
		Text_Name->SetText(Def->DisplayName);
	}

	if (Text_Description)
	{
		Text_Description->SetText(Def->Description);
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

	OnItemInfoUpdated(Item.GradeTag);
}

FText UGYItemInfoWidget::FormatMagnitude(const FRolledMagnitude& Magnitude, UDataTable* DisplayTable) const
{
	const FEnchantMagnitudeDisplayRow* Display = IsValid(DisplayTable)
		? DisplayTable->FindRow<FEnchantMagnitudeDisplayRow>(Magnitude.MagnitudeTag.GetTagName(), TEXT("FormatMagnitude"), false)
		: nullptr;

	const int32 Decimals = (Display != nullptr) ? Display->Decimals : 0;

	FNumberFormattingOptions NumberOptions;
	NumberOptions.MinimumFractionalDigits = Decimals;
	NumberOptions.MaximumFractionalDigits = Decimals;
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
