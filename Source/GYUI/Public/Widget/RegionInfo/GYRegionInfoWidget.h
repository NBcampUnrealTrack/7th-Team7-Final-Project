#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYRegionInfoWidget.generated.h"

class UCommonTextBlock;
class UImage;
struct FGYRegionEnteredMessage;
struct FGYWorldTimeMessage;

/**
 * 월드 정보 HUD - 세계 시간, 레벨, 지역명
 */
UCLASS()
class GYUI_API UGYRegionInfoWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** 세계 시간 */
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_WorldTime;
	/** 세계 레벨, 지역명 */
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_RegionName;
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_RegionLevel;
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UImage> Image_Region;

	UPROPERTY(EditDefaultsOnly, Category = "GY|WorldInfo")
	FText TimeFormat;        // "{0}:{1}"
	UPROPERTY(EditDefaultsOnly, Category = "GY|WorldInfo")
	FText RegionLevelFormat; // "Lv. {0}"

private:
	void HandleWorldTimeChanged(FGameplayTag Channel, const FGYWorldTimeMessage& Message);
	void HandleRegionEntered(FGameplayTag Channel, const FGYRegionEnteredMessage& Message);
};
