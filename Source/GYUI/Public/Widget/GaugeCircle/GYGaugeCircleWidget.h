#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYGaugeCircleWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
struct FGYAttributeValueMessage;
/**
 * 원형 게이지바 위젯
 */
UCLASS()
class GYUI_API UGYGaugeCircleWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_HP;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_Poise;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_SP;

	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	FName PercentParamName = TEXT("Percent");

	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	float HoldDuration = 1.5f;
	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	float FadeSpeed = 6.f;

private:
	void HandleStatMessage(FGameplayTag Channel, const FGYAttributeValueMessage& Message);
	void SetPercent(UImage* Image, float Percent);
	void NotifyActivity();

	float  CurrentAlpha = 0.f;
	double LastActivityTime = -1000.0;

	/** 머리티얼 인스턴스 기억해둘 캐시 맵 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, TObjectPtr<UMaterialInstanceDynamic>> MIDCache;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, float> LastPercent;
};

