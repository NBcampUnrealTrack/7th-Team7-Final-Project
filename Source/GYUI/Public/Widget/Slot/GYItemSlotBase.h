#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UI/GYUIMessages.h"
#include "GYItemSlotBase.generated.h"

class UImage;

// 아이템 슬롯 공통 베이스 — 아이콘 표시, 아이템 스냅샷 보관, 호버 정보 패널 발행.
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYItemSlotBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UGYItemSlotBase();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	// 아이템 표시: 아이콘 채우고 스냅샷 보관 후 OnViewChanged 호출
	void SetView(const FGYItemViewData& View);
	// 빈 칸 처리
	void ClearView();

	// 서브클래스가 카운트 텍스트/등급색 등 자체 비주얼을 갱신하는 훅
	virtual void OnViewChanged(bool bIsEmpty) {}

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UImage> Image_Icon;

	// 바인딩 시 등급 테두리 직접 제어
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UImage> Image_GradeBorder;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Slot")
	TMap<FGameplayTag, FLinearColor> GradeBorderColors;

	// 이 카테고리만 등급 테두리 표시
	UPROPERTY(EditDefaultsOnly, Category = "GY|Slot")
	FGameplayTag GradeBorderCategory;

	// 우클릭 시 정보 패널로 발행할 스냅샷 (Definition 비면 빈 칸)
	FGYItemViewData CurrentInfo;

private:
	void ApplyGradeBorder(const FGYItemViewData& View, bool bEmpty);

	// 정보 패널 발행. Channel=Show(호버 표시) / Pin(우클릭 토글)
	void BroadcastItemInfo(FGameplayTag Channel);
	// 빈 ViewData로 호버 종료 신호
	void BroadcastHideItemInfo();
};
