// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYLockOnIndicatorWidget.generated.h"

class UImage;
struct FGYLockOnMessage;

/**
 *
 */
UCLASS()
class GYUI_API UGYLockOnIndicatorWidget : public UGYUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> IndicatorIcon;

private:
	void HandleLockOnChanged(FGameplayTag Tag, const FGYLockOnMessage& Msg);
	bool IsMine(const FGYLockOnMessage& Msg) const;

	FGameplayMessageListenerHandle ChangedHandle;
	TWeakObjectPtr<AActor> Target;
};
