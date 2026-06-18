#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GYStatRowWidget.generated.h"

class UCommonTextBlock;

/**
 * 플레이어의 스탯 띄우는 위젯 중 한 줄
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYStatRowWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|StatRow")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|StatRow")
	bool bIntegerDisplay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|StatRow")
	bool bPercentDisplay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|StatRow")
	FLinearColor BonusColor = FLinearColor(0.4f, 1.0f, 0.4f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|StatRow")
	FLinearColor PenaltyColor = FLinearColor(1.0f, 0.4f, 0.4f);

	UFUNCTION(BlueprintCallable, Category = "GY|StatRow")
	void SetStat(const FText& InLabel, float BaseValue, float BonusValue);

	UFUNCTION(BlueprintCallable, Category = "GY|StatRow")
	void SetValues(float BaseValue, float BonusValue);

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Label;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Base;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Bonus;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|StatRow")
	void OnStatUpdated(float BaseValue, float BonusValue);

private:
	FText FormatValue(float Value) const;
};
