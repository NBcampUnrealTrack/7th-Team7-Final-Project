#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GYRevivalWidget.generated.h"

class UProgressBar;
struct FGYRevivalProgressMessage;

/**
 * 부활 위젯 - 진행 프로그래스바 존재
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYRevivalWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Progress;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Revival")
	float InterpSpeed = 12.f;

private:
	void HandleProgress(FGameplayTag Channel, const FGYRevivalProgressMessage& Msg);

	FGameplayMessageListenerHandle ListenerHandle;
	float TargetPercent = 0.f;
	float CurrentPercent = 0.f;
};
