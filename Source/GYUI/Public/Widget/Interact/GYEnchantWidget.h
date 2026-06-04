// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GYEnchantWidget.generated.h"

class UProgressBar;
class UButton;
class UGYEnchantSlotWidget;
/**
 *
 */
UCLASS()
class GYUI_API UGYEnchantWidget : public UGYActivatableWidget
{
	GENERATED_BODY()
public:
	UFUNCTION()
	void OnCurrencyChanged(FGameplayTag GameplayTag, int32 Amount);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGYEnchantSlotWidget> EnchantSlotWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExecuteButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;
private:
	UFUNCTION()
	void OnCloseButtonClicked();
	UFUNCTION()
	void OnExecuteButtonClicked();
	FDelegateHandle OnCurrencyChangedHandle;
};
