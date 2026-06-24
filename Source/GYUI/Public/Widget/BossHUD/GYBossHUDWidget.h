#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayEffectTypes.h"
#include "GYBossHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
struct FGYBossStateMessage;
struct FGYAttributeValueMessage;
struct FGYBossAOETimerMessage;

/**
 * 보스 HUD - 이름, 체력, Poise
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYBossHUDWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
	UGYBossHUDWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PoiseBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> AOEBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AOETimeText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void OnBossStateReceived(bool bVisible); // TODO : 연출

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void OnAOEWindowChanged(bool bActive, float Duration);

private:
	void HandleState(FGameplayTag, const FGYBossStateMessage& Msg);
	void BindToBoss(AActor* BossActor);
	void UnbindFromBoss();
	void UpdateHealthUI();
	void UpdatePoiseUI();

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnPoiseChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void OnBossReady(AGYEnemyCharacterBase* Boss);
	UFUNCTION()
	void OnBossDead(AGYEnemyCharacterBase* Boss);

	void InitializeBossData(AGYEnemyCharacterBase* Boss);

	FGameplayMessageListenerHandle StateHandle;

	TWeakObjectPtr<UAbilitySystemComponent> BossASC;
	TWeakObjectPtr<AGYEnemyCharacterBase> CurrentBoss;

	FDelegateHandle BossHealthHandle;
	FDelegateHandle BossMaxHealthHandle;
	FDelegateHandle BossPoiseHandle;
	FDelegateHandle BossMaxPoiseHandle;

	FTimerHandle InterpTimerHandle;

	bool bHealthInitialized = false;
	bool bPoiseInitialized = false;

	float TargetHealthPercent = 1.0f;
	float CurrentHealthPercent = 1.0f;
	float TargetPoisePercent = 1.0f;
	float CurrentPoisePercent = 1.0f;

	UPROPERTY(EditAnywhere, Category = "UI|Animation")
	float InterpSpeed = 5.0f;
	const float InterpTimerRate = 0.016f;

	void StartInterpTimer();
	void ProcessInterp();

	void HandleAOETimer(FGameplayTag, const FGYBossAOETimerMessage& Msg);
	void StartAOECountdown(float Duration);
	void StopAOECountdown();
	void TickAOECountdown();

	FGameplayMessageListenerHandle AOETimerHandle;
	FTimerHandle AOETickHandle;

	float AOEDuration = 0.f;
	float AOEEndTime  = 0.f;
	bool  bAOEActive  = false;

	const float AOETickRate = 0.05f;
};
