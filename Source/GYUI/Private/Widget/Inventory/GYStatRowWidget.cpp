#include "Widget/Inventory/GYStatRowWidget.h"
#include "CommonTextBlock.h"

void UGYStatRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Text_Label && !Label.IsEmpty())
	{
		Text_Label->SetText(Label);
	}
}

void UGYStatRowWidget::SetStat(const FText& InLabel, float BaseValue, float BonusValue)
{
	Label = InLabel;
	if (Text_Label)
	{
		Text_Label->SetText(InLabel);
	}
	SetValues(BaseValue, BonusValue);
}

void UGYStatRowWidget::SetValues(float BaseValue, float BonusValue)
{
	if (Text_Base)
	{
		Text_Base->SetText(FormatValue(BaseValue));
	}

	if (Text_Bonus)
	{
		if (FMath::Abs(BonusValue) < 0.01f)
		{
			Text_Bonus->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			const FText Sign = BonusValue >= 0.f ? INVTEXT("+") : INVTEXT("-");
			const FText Magnitude = FormatValue(FMath::Abs(BonusValue));
			Text_Bonus->SetText(FText::Format(INVTEXT("({0}{1})"), Magnitude, Sign));
			Text_Bonus->SetColorAndOpacity(BonusValue >= 0.f ? BonusColor : PenaltyColor);
			Text_Bonus->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	OnStatUpdated(BaseValue, BonusValue);
}

FText UGYStatRowWidget::FormatValue(float Value) const
{
	if (bPercentDisplay)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 1;
		return FText::Format(INVTEXT("{0}%"), FText::AsNumber(Value * 100.f, &Options));
	}

	if (bIntegerDisplay)
	{
		return FText::AsNumber(FMath::RoundToInt32(Value));
	}
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = 1;
	return FText::AsNumber(Value, &Options);
}
