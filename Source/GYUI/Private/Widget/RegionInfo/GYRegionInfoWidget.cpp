#include "Widget/RegionInfo/GYRegionInfoWidget.h"
#include "CommonTextBlock.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "Components/Image.h"

#define LOCTEXT_NAMESPACE "GYUI"

void UGYRegionInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TimeFormat.IsEmpty())
	{
		TimeFormat = NSLOCTEXT("GYUI", "WorldTimeFormat", "{0}:{1}");
	}
	if (RegionLevelFormat.IsEmpty())
	{
		RegionLevelFormat = NSLOCTEXT("GYUI", "RegionLevelFormat", "Lv. {0}");
	}

	// 세계 레벨 메세지 구독
	ListenForMessage<UGYRegionInfoWidget, FGYRegionEnteredMessage>(
		GYGameplayTags::Message_Region_Entered, this, &UGYRegionInfoWidget::HandleRegionEntered);
	// 세계 시간 메세지 구독
	ListenForMessage<UGYRegionInfoWidget, FGYWorldTimeMessage>(
		GYGameplayTags::Message_World_TimeChanged, this, &UGYRegionInfoWidget::HandleWorldTimeChanged);
}

void UGYRegionInfoWidget::HandleWorldTimeChanged(FGameplayTag Channel, const FGYWorldTimeMessage& Message)
{
	if (!Text_WorldTime) return;

	FNumberFormattingOptions Pad;
	Pad.MinimumIntegralDigits = 2;
	Text_WorldTime->SetText(FText::Format(TimeFormat,
		FText::AsNumber(Message.Hours, &Pad),
		FText::AsNumber(Message.Minutes, &Pad)));
}

void UGYRegionInfoWidget::HandleRegionEntered(FGameplayTag, const FGYRegionEnteredMessage& Message)
{
	APawn* LocalPawn = GetOwningPlayerPawn();
	if (!LocalPawn || Message.Pawn.Get() != LocalPawn)
	{
		return;
	}

	if (Text_RegionName)
	{
		Text_RegionName->SetText(Message.RegionDisplayName);
	}
	if (Text_RegionLevel)
	{
		Text_RegionLevel->SetText(FText::Format(RegionLevelFormat, FText::AsNumber(Message.RegionLevel)));
	}
	if (Image_Region)
	{
		if (!Message.RegionIcon.IsNull())
		{
			Image_Region->SetBrushFromSoftTexture(Message.RegionIcon, false);
			Image_Region->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image_Region->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

#undef LOCTEXT_NAMESPACE
