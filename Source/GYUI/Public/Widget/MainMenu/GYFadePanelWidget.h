#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYFadePanelWidget.generated.h"

class UButton;
class UWidgetAnimation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGYFadePanelClosed);

/**
 * 페이드 인/아웃 오버레이 패널 (크레딧, 세팅 등)
 */
UCLASS(Blueprintable)
class GYUI_API UGYFadePanelWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GY|Panel")
	void OpenPanel();

	UFUNCTION(BlueprintCallable, Category = "GY|Panel")
	void ClosePanel();

	UFUNCTION(BlueprintPure, Category = "GY|Panel")
	bool IsPanelOpen() const { return bIsOpen; }

	UPROPERTY(BlueprintAssignable, Category = "GY|Panel")
	FGYFadePanelClosed OnPanelClosed;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> FadeAnim;

	/** 있으면 자동으로 ClosePanel에 연결 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Back;

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleFadeFinished();

private:
	bool bIsOpen = false;
	bool bClosing = false;

	FWidgetAnimationDynamicEvent FadeFinishedDelegate;
};
