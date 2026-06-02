#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYLootDropSlotWidget.generated.h"

class UImage;
class UCommonTextBlock;
class ALootBoxActor;
struct FLootDrop;

UCLASS(Abstract, Blueprintable)
class GYUI_API UGYLootDropSlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetDrop(ALootBoxActor* InBox, int32 InDropIndex, const FLootDrop& Drop);

protected:
	// BP 클릭/선택 핸들러에서 호출 — 이 드롭 줍기 요청
	UFUNCTION(BlueprintCallable, Category = "GY|Loot")
	void RequestTake();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Count;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Loot")
	void OnDropUpdated(FGameplayTag GradeTag, int32 Count);

private:
	TWeakObjectPtr<ALootBoxActor> BoundBox;
	int32 DropIndex = INDEX_NONE;
};
