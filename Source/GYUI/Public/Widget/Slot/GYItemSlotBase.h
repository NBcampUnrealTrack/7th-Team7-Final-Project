#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UI/GYUIMessages.h"
#include "GYItemSlotBase.generated.h"

class UImage;

// 아이템 슬롯 공통 베이스 — 아이콘 표시, 아이템 스냅샷 보관, 우클릭 정보 패널 발행.
// 인벤/루트/장비/인첸트 슬롯이 상속하며, 출처(FInventoryEntry/FLootDrop 등)를 FGYItemViewData로 변환해 SetView만 호출하면 된다.
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYItemSlotBase : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 아이템 표시: 아이콘 채우고 스냅샷 보관 후 OnViewChanged 호출
	void SetView(const FGYItemViewData& View);
	// 빈 칸 처리
	void ClearView();
	// 현재 아이템 정보를 정보 패널로 발행 (우클릭 등)
	void BroadcastItemInfo();

	// 서브클래스가 카운트 텍스트/등급색 등 자체 비주얼을 갱신하는 훅
	virtual void OnViewChanged(bool bIsEmpty) {}

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UImage> Image_Icon;

	// 우클릭 시 정보 패널로 발행할 스냅샷 (Definition 비면 빈 칸)
	FGYItemViewData CurrentInfo;
};
