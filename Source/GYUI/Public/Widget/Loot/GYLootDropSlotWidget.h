#pragma once

#include "CoreMinimal.h"
#include "Widget/Slot/GYItemSlotBase.h"
#include "GameplayTagContainer.h"
#include "GYLootDropSlotWidget.generated.h"

class UCommonTextBlock;
class ALootBoxActor;
struct FLootDrop;

UCLASS(Abstract, Blueprintable)
class GYUI_API UGYLootDropSlotWidget : public UGYItemSlotBase
{
	GENERATED_BODY()

public:
	void SetDrop(ALootBoxActor* InBox, int32 InDropIndex, const FLootDrop& Drop);
	void SetEmpty();

protected:
	virtual void OnViewChanged(bool bIsEmpty) override;

	// BP 클릭/선택 핸들러에서 호출 — 이 드롭 줍기 요청
	UFUNCTION(BlueprintCallable, Category = "GY|Loot")
	void RequestTake();

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Count;

	// 빈 칸도 이 이벤트 재사용 — 빈 태그 전달 시 그래프 Switch의 Default가 등급 프레임을 숨김
	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Loot")
	void OnDropUpdated(FGameplayTag GradeTag, int32 Count);

private:
	TWeakObjectPtr<ALootBoxActor> BoundBox;
	int32 DropIndex = INDEX_NONE;
};
