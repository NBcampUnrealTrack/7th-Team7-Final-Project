#include "Widget/WorldReset/GYWorldResetWidget.h"
#include "Components/Image.h"

UGYWorldResetWidget::UGYWorldResetWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UGYWorldResetWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyVisuals(0.f, 0.f);
	ResetHands();
}

void UGYWorldResetWidget::PlayResetSequence(float DurationOverride)
{
    if (DurationOverride > KINDA_SMALL_NUMBER)
    {
        HoldDuration = DurationOverride;
    }

    ResetHands();
    EnterPhase(EPhase::FadeIn);
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGYWorldResetWidget::EnterPhase(EPhase NewPhase)
{
    Phase = NewPhase;
    PhaseElapsed = 0.f;
}

void UGYWorldResetWidget::ResetHands()
{
    MinuteAngle = 0.f;
    HourAngle = 0.f;
    if (Image_HourHand) Image_HourHand->SetRenderTransformAngle(0.f);
    if (Image_MinuteHand) Image_MinuteHand->SetRenderTransformAngle(0.f);
}

void UGYWorldResetWidget::ApplyVisuals(float BgOpacity, float ContentOpacity)
{
    if (Image_Background)
    {
        Image_Background->SetRenderOpacity(BgOpacity * BackgroundMaxOpacity);
    }
    if (Image_ClockFace) Image_ClockFace->SetRenderOpacity(ContentOpacity);
    if (Image_HourHand) Image_HourHand->SetRenderOpacity(ContentOpacity);
    if (Image_MinuteHand) Image_MinuteHand->SetRenderOpacity(ContentOpacity);
}

void UGYWorldResetWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (Phase == EPhase::Idle) return;

    PhaseElapsed += InDeltaTime;

    MinuteAngle = FMath::Fmod(MinuteAngle - MinuteHandSpeedDeg * InDeltaTime, 360.f);
    HourAngle = FMath::Fmod(HourAngle - HourHandSpeedDeg * InDeltaTime, 360.f);
    if (Image_MinuteHand) Image_MinuteHand->SetRenderTransformAngle(MinuteAngle);
    if (Image_HourHand) Image_HourHand->SetRenderTransformAngle(HourAngle);

    switch (Phase)
    {
    case EPhase::FadeIn:
    {
        const float Alpha = (FadeInDuration > 0.f) ? FMath::Clamp(PhaseElapsed / FadeInDuration, 0.f, 1.f) : 1.f;
        ApplyVisuals(Alpha, Alpha);
        if (Alpha >= 1.f)
        {
            EnterPhase(EPhase::Hold);
        }
        break;
    }
    case EPhase::Hold:
    {
        ApplyVisuals(1.f, 1.f);
        if (PhaseElapsed >= HoldDuration)
        {
            EnterPhase(EPhase::FadeOut);
        }
        break;
    }
    case EPhase::FadeOut:
    	{
    		const float T = (FadeOutDuration > 0.f) ? FMath::Clamp(PhaseElapsed / FadeOutDuration, 0.f, 1.f) : 1.f;
    		const float Alpha = 1.f - T;
    		ApplyVisuals(Alpha, Alpha);
    		if (T >= 1.f)
    		{
    			ApplyVisuals(0.f, 0.f);
    			ResetHands();
    			EnterPhase(EPhase::Idle);
    			OnSequenceFinished.Broadcast();
    		}
    		break;
    	}
    default: break;
    }
}
