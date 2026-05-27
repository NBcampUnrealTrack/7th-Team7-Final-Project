#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYStatBarWidget.generated.h"

class UProgressBar;
class UCommonRichTextBlock;
struct FGYAttributeValueMessage;

/**
 * 스탯 바 위젯 - Current, Max 한 쌍
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYStatBarWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Progress;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonRichTextBlock> Text_Value;

	UPROPERTY(EditDefaultsOnly, Category = "GY|StatBar")
	FGameplayTag StatChannelTag;

	UPROPERTY(EditDefaultsOnly, Category = "GY|StatBar")
	FText ValueFormat;

	UPROPERTY(EditDefaultsOnly, Category = "GY|StatBar", meta = (ClampMin = 0, ClampMax = 4))
	int32 NumberPrecision = 0;

	UPROPERTY(EditDefaultsOnly, Category = "GY|StatBar")
	FLinearColor BarColor = FLinearColor::White;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|StatBar")
	void OnStatUpdated(float OldCurrent, float NewCurrent, float NewMax);

private:
	/** 스탯 브로드캐스트 수신 시 실행 함수 */
	void HandleAttributeMessage(FGameplayTag Channel, const FGYAttributeValueMessage& Message);
	void UpdateVisuals();

	float CachedCurrent = 0.f;
	float CachedMax = 1.f;
};
