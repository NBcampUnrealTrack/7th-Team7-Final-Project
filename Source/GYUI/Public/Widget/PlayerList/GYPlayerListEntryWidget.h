#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYPlayerListEntryWidget.generated.h"

class APlayerState;
class UProgressBar;
class UCommonTextBlock;
struct FGYPlayerNameMessage;

/**
 * 플레이어 목록 한 행 - 이름, 레벨, 체력 표시
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYPlayerListEntryWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
	void InitializeFromPlayerState(APlayerState* PS);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> PlayerNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> LevelText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;
	/** ASC Rep 지연 시 재시도하는 간격 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI", meta = (ClampMin = "0.05"))
	float ASCRetryInterval = 0.1f;
	/** 데이터 갱신 시 추가 처리할 할 수 있는 용도 */
	UFUNCTION(BlueprintImplementableEvent, Category = "GY|UI")
	void OnEntryUpdated(const FString& Name, int32 Level, float Health, float MaxHealth);

private:
	TWeakObjectPtr<APlayerState> TrackedPS;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	FTimerHandle ASCRetryHandle;

	void TryConnectASC();
	void BindToASC(UAbilitySystemComponent* InASC);
	void RefreshAll();
	void HandlePlayerNameMessage(FGameplayTag Channel, const FGYPlayerNameMessage& Message);
};
