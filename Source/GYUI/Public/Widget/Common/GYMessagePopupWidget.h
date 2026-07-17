#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GYMessagePopupWidget.generated.h"

class UButton;
class UCommonTextBlock;

/**
 * 공용 안내, 확인 팝업
 *  공지 모드 - 닫기 버튼만
 *  확인 모드 - 네/아니오 버튼
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYMessagePopupWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYMessagePopupWidget(const FObjectInitializer& ObjectInitializer);

	void SetupNotice(const FText& InTitle, const FText& InMessage);
	void SetupConfirm(const FText& InTitle, const FText& InMessage);

	FSimpleMulticastDelegate OnConfirmed;
	FSimpleMulticastDelegate OnCancelled;
	FSimpleMulticastDelegate OnClosed;

	static UGYMessagePopupWidget* ShowNotice(const UObject* WorldContext, const FText& Title, const FText& Message);
	static UGYMessagePopupWidget* ShowConfirm(const UObject* WorldContext, const FText& Title, const FText& Message);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Title;
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_Message;
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UButton> Button_Close;
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UButton> Button_Confirm;
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UButton> Button_Cancel;

private:
	UFUNCTION() void HandleCloseClicked();
	UFUNCTION() void HandleConfirmClicked();
	UFUNCTION() void HandleCancelClicked();
	void CloseSelf();

	static UGYMessagePopupWidget* ShowInternal(const UObject* WorldContext, const FText& Title,
	                                           const FText& Message, bool bConfirm);
};
