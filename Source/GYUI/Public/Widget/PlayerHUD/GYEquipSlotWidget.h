#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYEquipSlotWidget.generated.h"

class UImage;
class UTexture2D;
struct FGYEquipSlotMessage;

/**
 * 장비 슬롯 - 무기, 방어구
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYEquipSlotWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Equip")
	FGameplayTag SlotTag;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Equip")
	TSoftObjectPtr<UTexture2D> EmptySlotIcon;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Equip")
	void OnEquipSlotUpdated(bool bIsEmpty);

private:
	/** 장비 변경 메세지 수신 시 호출 함수 */
	void HandleEquipMessage(FGameplayTag Channel, const FGYEquipSlotMessage& Message);
	void DisplayEmpty();
};
