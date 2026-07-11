#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "GYTimeRiftPanelWidget.generated.h"

/**
 * 시간의 틈 계열 위젯 공통 베이스.
 * ESC(뒤로가기) 입력 시 GetExitEventTag() 가 반환하는 이벤트를 서버로 전송한다.
 *  - 선택창(TimeRift)      : Event.TimeRift.Exit        → 선택창 닫힘
 *  - 제단/리롤/스킬트리     : 각 *.Exit                  → 닫히고 선택창으로 복귀
 */
UCLASS(Abstract)
class GYUI_API UGYTimeRiftPanelWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYTimeRiftPanelWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool NativeOnHandleBackAction() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** ESC/뒤로가기 시 서버로 보낼 종료 이벤트 태그. 자식이 지정. (빈 태그면 아무 동작 안 함) */
	virtual FGameplayTag GetExitEventTag() const { return FGameplayTag(); }

	/** 종료 이벤트를 서버로 전송 — 닫기 버튼 핸들러에서도 재사용 */
	void RequestExit();
};
