#include "Widget/GYStatBarWidget.h"
#include "CommonRichTextBlock.h"
#include "Components/ProgressBar.h"
#include "UI/GYUIMessages.h"

void UGYStatBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Bar_Progress)
	{
		Bar_Progress->SetFillColorAndOpacity(BarColor);
	}
}

void UGYStatBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Bar_Progress)
	{
		Bar_Progress->SetPercent(0.f);
	}

	if (ValueFormat.IsEmpty())
	{
		ValueFormat = NSLOCTEXT("GYUI", "StatBarDefaultFormat", "{0} / {1}");
	}

	if (!StatChannelTag.IsValid()) return;

	// 스탯 태그 구독 - 스탯 변동 시 함수 호출
	ListenForMessage<UGYStatBarWidget, FGYAttributeValueMessage>(StatChannelTag, this,
	                                                             &UGYStatBarWidget::HandleAttributeMessage);
}

void UGYStatBarWidget::HandleAttributeMessage(FGameplayTag, const FGYAttributeValueMessage& Message)
{
	const float OldCurrent = CachedCurrent;

	CachedCurrent = Message.CurrentValue;
	CachedMax = Message.MaxValue;
	UpdateVisuals();

	OnStatUpdated(OldCurrent, CachedCurrent, CachedMax);
}

void UGYStatBarWidget::UpdateVisuals()
{
	const float SafeMax = FMath::Max(CachedMax, KINDA_SMALL_NUMBER);
	const float Percent = FMath::Clamp(CachedCurrent / SafeMax, 0.f, 1.f);

	if (Bar_Progress)
	{
		Bar_Progress->SetPercent(Percent);
	}

	if (Text_Value)
	{
		FNumberFormattingOptions Opts;
		Opts.MinimumFractionalDigits = NumberPrecision;
		Opts.MaximumFractionalDigits = NumberPrecision;

		Text_Value->SetText(FText::Format(ValueFormat, FText::AsNumber(CachedCurrent, &Opts),
		                                  FText::AsNumber(CachedMax, &Opts)));
	}
}
