// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GYTimeRiftWidget.generated.h"

class UButton;
/**
 *
 */
UCLASS()
class GYUI_API UGYTimeRiftWidget : public UGYActivatableWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AltarButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EnchantButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SkillTreeButton;

private:
	UFUNCTION()
	void  OnExitButtonClicked();
	UFUNCTION()
	void  OnRestButtonClicked();
	UFUNCTION()
	void  OnAltarButtonClicked();
	UFUNCTION()
	void  OnEnchantButtonClicked();
	UFUNCTION()
	void  OnSkillTreeButtonClicked();

};
