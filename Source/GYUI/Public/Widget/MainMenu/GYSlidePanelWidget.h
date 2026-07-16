#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYSlidePanelWidget.generated.h"

class UWidgetAnimation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGYSlidePanelClosed);

/**
 * 우->좌 슬라이드 인/아웃 패널 공통 베이스
 */
UCLASS(Abstract)
class GYUI_API UGYSlidePanelWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
	/** 화면 안으로 슬라이드 인 */
	UFUNCTION(BlueprintCallable, Category = "GY|Panel")
	void OpenPanel();

	/** 화면 밖으로 슬라이드 아웃 */
	UFUNCTION(BlueprintCallable, Category = "GY|Panel")
	void ClosePanel();

	/** 열려 있으면 닫고, 닫혀 있으면 염 */
	UFUNCTION(BlueprintCallable, Category = "GY|Panel")
	void TogglePanel();

	UFUNCTION(BlueprintPure, Category = "GY|Panel")
	bool IsPanelOpen() const { return bIsOpen; }

	/** 슬라이드 아웃이 끝난 뒤 브로드캐스트 */
	UPROPERTY(BlueprintAssignable, Category = "GY|Panel")
	FGYSlidePanelClosed OnPanelClosed;

protected:
	virtual void NativeConstruct() override;

	/** 이름이 반드시 SlideAnim 이어야 바인딩됨 */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> SlideAnim;

	UFUNCTION()
	void HandleSlideFinished();

private:
	bool bIsOpen = false;
	bool bClosing = false;

	FWidgetAnimationDynamicEvent SlideFinishedDelegate;
};
