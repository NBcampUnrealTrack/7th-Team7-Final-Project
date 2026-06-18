#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GYWorldResetWidget.generated.h"

class UImage;
DECLARE_MULTICAST_DELEGATE(FOnWorldResetSequenceFinished);

UCLASS(Abstract, Blueprintable)
class GYUI_API UGYWorldResetWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UGYWorldResetWidget(const FObjectInitializer& ObjectInitializer);

	void PlayResetSequence(float DurationOverride);

	FOnWorldResetSequenceFinished OnSequenceFinished;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry&, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Image_Background;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_ClockFace;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Image_HourHand;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Image_MinuteHand;

    UPROPERTY(EditDefaultsOnly, Category = "GY|WorldReset", meta = (ClampMin = 0.0))
    float FadeInDuration = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "GY|WorldReset", meta = (ClampMin = 0.0))
    float HoldDuration = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "GY|WorldReset", meta = (ClampMin = 0.0))
    float FadeOutDuration = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "GY|WorldReset")
    float MinuteHandSpeedDeg = 720.f;

    UPROPERTY(EditDefaultsOnly, Category = "GY|WorldReset")
    float HourHandSpeedDeg = 60.f;

    UPROPERTY(EditDefaultsOnly, Category = "GY|WorldReset", meta = (ClampMin = 0.0, ClampMax = 1.0))
    float BackgroundMaxOpacity = 1.0f;

private:
	enum class EPhase : uint8
	{
		Idle, FadeIn, Hold, FadeOut
	};
	EPhase Phase = EPhase::Idle;
	float PhaseElapsed = 0.f;
	float MinuteAngle = 0.f;
	float HourAngle = 0.f;

	void EnterPhase(EPhase NewPhase);
	void ApplyVisuals(float BgOpacity, float ContentOpacity);
	void ResetHands();
};
