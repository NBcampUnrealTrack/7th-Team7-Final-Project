#pragma once

#include "CoreMinimal.h"
#include "Widget/MainMenu/GYSlidePanelWidget.h"
#include "GYSessionCreateWidget.generated.h"

class UButton;
class UEditableTextBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGYSessionCreated);

// 세션(월드) 생성 패널 — 이름 입력 후 생성. 생성 = 자동 참여이므로 성공 시 목록 갱신용 델리게이트만 쏘고 닫는다.
UCLASS(Blueprintable)
class GYUI_API UGYSessionCreateWidget : public UGYSlidePanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GY|Session")
	FGYSessionCreated OnSessionCreated;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Create;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Cancel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> SessionTextBox;

private:
	UFUNCTION() void HandleCreateClicked();
	UFUNCTION() void HandleCancelClicked();

	void OnCreateComplete(bool bSuccess, int64 WorldId);
};
