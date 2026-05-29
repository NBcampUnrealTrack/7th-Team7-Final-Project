#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYPotionSlotWidget.generated.h"

class UImage;
class UCommonTextBlock;
class UTexture2D;
struct FGYPotionSlotMessage;

/**
 * 포션용 슬롯
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYPotionSlotWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Count;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Potion")
	FGameplayTag ChargePoolTag;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Potion")
	TSoftObjectPtr<UTexture2D> EmptySlotIcon;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Potion")
	void OnPotionSlotUpdated(int32 NewStackCount, bool bIsEmpty);

private:
	/** 포션 정보 변경 메시지 수신 처리 함수 */
	void HandlePotionMessage(FGameplayTag Channel, const FGYPotionSlotMessage& Message);
	/** 비어있는 상태 비주얼로 초기화 함수 */
	void DisplayEmpty();
};
