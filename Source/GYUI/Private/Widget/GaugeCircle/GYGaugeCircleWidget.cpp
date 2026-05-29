#include "Widget/GaugeCircle/GYGaugeCircleWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

void UGYGaugeCircleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// GMS로 태그 신호 맞춤
	ListenForMessage<UGYGaugeCircleWidget, FGYAttributeValueMessage>(GYGameplayTags::Message_Stat_Health, this,
	                                                                 &UGYGaugeCircleWidget::HandleStatMessage);
	ListenForMessage<UGYGaugeCircleWidget, FGYAttributeValueMessage>(GYGameplayTags::Message_Stat_Poise, this,
	                                                                 &UGYGaugeCircleWidget::HandleStatMessage);
	ListenForMessage<UGYGaugeCircleWidget, FGYAttributeValueMessage>(GYGameplayTags::Message_Stat_Stamina, this,
	                                                                 &UGYGaugeCircleWidget::HandleStatMessage);

	SetRenderOpacity(0.f);
}

void UGYGaugeCircleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 대기 시간 체크해서 투명도 처리
	const UWorld* World = GetWorld();
	const float Target = (World && World->TimeSince(LastActivityTime) <= HoldDuration) ? 1.f : 0.f;

	if (Target == 1.f)
	{
		CurrentAlpha = 1.f;
	}
	else
	{
		CurrentAlpha = FMath::FInterpTo(CurrentAlpha, Target, InDeltaTime, FadeSpeed);
	}
	SetRenderOpacity(CurrentAlpha);
}

void UGYGaugeCircleWidget::HandleStatMessage(FGameplayTag Channel, const FGYAttributeValueMessage& Message)
{
	const float Ratio = FMath::Clamp(
		Message.CurrentValue / FMath::Max(Message.MaxValue, KINDA_SMALL_NUMBER), -1.f, 1.f);

	if (Channel == GYGameplayTags::Message_Stat_Health) SetPercent(Image_HP, Ratio);
	else if (Channel == GYGameplayTags::Message_Stat_Poise) SetPercent(Image_Poise, Ratio);
	else if (Channel == GYGameplayTags::Message_Stat_Stamina) SetPercent(Image_SP, Ratio);
}

void UGYGaugeCircleWidget::SetPercent(UImage* Image, float Percent)
{
	if (!Image) return;

	const bool bHadPrev = LastPercent.Contains(Image); // 저장된 값 있는지 확인
	if (bHadPrev && FMath::IsNearlyEqual(LastPercent[Image], Percent, 0.001f)) return;

	TObjectPtr<UMaterialInstanceDynamic>* Found = MIDCache.Find(Image);
	UMaterialInstanceDynamic* MID = Found ? Found->Get() : nullptr;

	if (!MID)
	{
		MID = Image->GetDynamicMaterial();
		if (!MID) return;
		MIDCache.Add(Image, MID);
	}
	MID->SetScalarParameterValue(PercentParamName, Percent);

	const bool bFirstBaseline = !bHadPrev; // 처음으로 값 셋팅되는 순간인지 판별
	LastPercent.Add(Image, Percent);

	if (!bFirstBaseline)
	{
		NotifyActivity();
	}
}

void UGYGaugeCircleWidget::NotifyActivity()
{
	if (const UWorld* World = GetWorld())
		LastActivityTime = World->GetTimeSeconds();
}
