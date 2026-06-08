#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GYBossHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
struct FGYBossStateMessage;
struct FGYAttributeValueMessage;

/**
 * 보스 HUD - 이름, 체력, Poise
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYBossHUDWidget : public UGYUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PoiseBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossNameText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void OnBossStateReceived(bool bVisible); // TODO : 연출

private:
	void HandleState(FGameplayTag, const FGYBossStateMessage& Msg);
	void HandleHealth(FGameplayTag, const FGYAttributeValueMessage& Msg);
	void HandlePoise(FGameplayTag, const FGYAttributeValueMessage& Msg);

	FGameplayMessageListenerHandle StateHandle;
	FGameplayMessageListenerHandle HealthHandle;
	FGameplayMessageListenerHandle PoiseHandle;
};
