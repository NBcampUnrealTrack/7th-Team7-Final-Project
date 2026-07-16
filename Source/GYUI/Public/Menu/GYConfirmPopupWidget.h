#pragma once

#include "Blueprint/UserWidget.h"
#include "GYConfirmPopupWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DELEGATE(FGYOnConfirmed);

// 범용 확인 모달 — 제목/본문 + [확인][취소]. 확인 시 델리게이트 실행, 어느 쪽이든 스스로 닫힌다.
// WBP 로 꾸미려면 이 클래스를 부모로 reparent 하고 아래 이름의 위젯을 두면 채택된다:
//   TitleText / BodyText (TextBlock), ConfirmButton / CancelButton (Button)
UCLASS()
class GYUI_API UGYConfirmPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 표시 직전 호출 — 문구와 확인 콜백 지정 (버튼 라벨은 WBP 디자인 소유)
	void SetupConfirm(const FText& Title, const FText& Body, FGYOnConfirmed OnConfirm);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	// BindWidgetOptional: WBP 가 같은 이름의 위젯 변수를 두면 여기에 바인딩 (BP 프로퍼티 중복 생성 충돌 방지)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelButton;

	// WBP 파생은 멤버 바인딩이 NativeConstruct 시점 — Setup 이 먼저 와도 안전하게 캐시 후 재적용
	void ApplyTexts();

	FText CachedTitle;
	FText CachedBody;
	FGYOnConfirmed OnConfirmed;
};
