#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "UI/GYUIMessages.h"
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
	void SetEmpty();

protected:
	// 우클릭 → 아이템 정보 패널 표시
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// BP 클릭/선택 핸들러에서 호출 — 이 드롭 줍기 요청
	UFUNCTION(BlueprintCallable, Category = "GY|Loot")
	void RequestTake();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Count;

	// 빈 칸도 이 이벤트 재사용 — 빈 태그 전달 시 그래프 Switch의 Default가 등급 프레임을 숨김
	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Loot")
	void OnDropUpdated(FGameplayTag GradeTag, int32 Count);

private:
	TWeakObjectPtr<ALootBoxActor> BoundBox;
	int32 DropIndex = INDEX_NONE;

	// 우클릭 시 정보 패널로 발행할 스냅샷 (Definition 비면 빈 칸)
	FGYItemViewData CurrentInfo;
};
