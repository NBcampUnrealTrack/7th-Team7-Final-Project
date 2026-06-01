#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYItemSlotWidget.generated.h"

class UImage;
class UCommonTextBlock;
struct FInventoryEntry;

UCLASS(Abstract, Blueprintable)
class GYUI_API UGYItemSlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetEntry(const FInventoryEntry& Entry);
	void SetEmpty();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_StackCount;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Inventory")
	void OnSlotUpdated(bool bIsEmpty, FGameplayTag GradeTag, int32 StackCount);
};
