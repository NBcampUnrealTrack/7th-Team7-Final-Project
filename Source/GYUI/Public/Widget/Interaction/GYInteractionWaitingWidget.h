#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GYInteractionWaitingWidget.generated.h"

class UTextBlock;
struct FGYInteractionWaitingMessage;

/**
 * 상호작용 후 플레이어 대기 위젯
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYInteractionWaitingWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYInteractionWaitingWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "GY|Ending")
	void SetCount(int32 Current, int32 Required);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;
	UPROPERTY(EditDefaultsOnly, Category = "Waiting")
	FText CountFormat;
	UFUNCTION(BlueprintImplementableEvent, Category = "Waiting")
	void OnCountUpdated(int32 Current, int32 Required);

private:
	void HandleWaitingMessage(FGameplayTag, const FGYInteractionWaitingMessage& Msg);

	FGameplayMessageListenerHandle ListenerHandle;
};
