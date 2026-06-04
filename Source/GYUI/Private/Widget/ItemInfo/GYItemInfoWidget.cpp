#include "Widget/ItemInfo/GYItemInfoWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Enchant/EnchantOptionResolver.h"
#include "Items/EnchantOptionRow.h"
#include "Items/ItemDefinition.h"
#include "UI/GYUIMessages.h"

void UGYItemInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 첫 우클릭 전엔 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	if (UWorld* World = GetWorld())
	{
		ListenerHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(
			GYGameplayTags::Message_UI_ShowItemInfo,
			this,
			&UGYItemInfoWidget::HandleShowItemInfo);
	}
}

void UGYItemInfoWidget::NativeDestruct()
{
	if (ListenerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayMessageSubsystem::Get(World).UnregisterListener(ListenerHandle);
		}
		ListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::NativeDestruct();
}

void UGYItemInfoWidget::HandleShowItemInfo(FGameplayTag, const FGYItemViewData& Item)
{
	UItemDefinition* Def = Item.Definition.LoadSynchronous();
	if (!IsValid(Def))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

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
		TArray<FText> Lines;
		for (const FRolledEnchantOption& Option : Item.RolledOptions)
		{
			const FEnchantOptionRow* Row = EnchantOptionResolver::FindRow(Def, Option.OptionId);
			const FText Label = (Row != nullptr && !Row->DisplayName.IsEmpty()) ? Row->DisplayName : FText::FromName(Option.OptionId);

			FString ValueText;
			for (const FRolledMagnitude& Magnitude : Option.Magnitudes)
			{
				if (!ValueText.IsEmpty()) ValueText += TEXT(", ");
				ValueText += FString::Printf(TEXT("+%g"), Magnitude.Value);
			}

			Lines.Add(ValueText.IsEmpty()
				? Label
				: FText::Format(INVTEXT("{0} {1}"), Label, FText::FromString(ValueText)));
		}

		Text_EnchantOptions->SetText(FText::Join(INVTEXT("\n"), Lines));
		Text_EnchantOptions->SetVisibility(Lines.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	OnItemInfoUpdated(Item.GradeTag);
}
