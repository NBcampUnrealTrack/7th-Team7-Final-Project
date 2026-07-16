#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GYMainMenuWidget.generated.h"

class UButton;
class UWidget;
class UCommonActivatableWidget;
class UGYSessionContainerWidget;
class UGYSessionCreateWidget;
class UGYFadePanelWidget;

UCLASS(Blueprintable)
class GYUI_API UGYMainMenuWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Session;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Setting;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Credit;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_GameExit;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYSessionContainerWidget> WBP_SessionContainer;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYSessionCreateWidget>    WBP_SessionCreate;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGYFadePanelWidget> WBP_DevCredit;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> ContentRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|MainMenu")
	TSubclassOf<UCommonActivatableWidget> SettingsWidgetClass;

private:
	UFUNCTION() void HandleSessionClicked();
	UFUNCTION() void HandleSettingClicked();
	UFUNCTION() void HandleCreditClicked();
	UFUNCTION() void HandleGameExitClicked();

	UFUNCTION() void HandleCreateSessionRequested();
	UFUNCTION() void HandleCreditClosed();

	void HandleSettingsClosed();

	/** 현재 떠 있는 세팅 위젯 인스턴스 */
	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidget> ActiveSettings;
};
