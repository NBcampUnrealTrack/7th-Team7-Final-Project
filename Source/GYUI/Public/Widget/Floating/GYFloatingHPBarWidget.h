#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYFloatingHPBarWidget.generated.h"

class UProgressBar;
class UCommonTextBlock;
class UAbilitySystemComponent;
class APlayerState;
struct FOnAttributeChangeData;
struct FGYPlayerNameMessage;
struct FGYCharacterReadyMessage;

/**
 * World 기준으로 캐릭터 머리 위에서 표시되는 체력바
 * owner 종류에 따라 동작이 다름 - 로컬 플레이어는 안뜨게, 다른 플레이어는 +닉네임해서 항상 뜨게, 적은 변동 시만 뜨게
 */
UCLASS()
class GYUI_API UGYFloatingHPBarWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
	UGYFloatingHPBarWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void ResetWidgetState();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> NameText;

	/** AI 모드에서 마지막 변동 후 유지 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI")
	float HoldDuration = 2.0f;

	/** AI 모드에서 페이드 아웃 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI")
	float FadeSpeed = 4.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|UI")
	void OnHealthUpdated(float Current, float Max);

	/** 다른 플레이어 체력바 색상 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Color")
	FLinearColor PlayerHPColor = FLinearColor::Blue;

	/** 적 체력바 색상 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Color")
	FLinearColor EnemyHPColor = FLinearColor::Red;

	/** 중간 HP 구간 색상 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Color")
	FLinearColor MidHealthColor = FLinearColor(0.95f, 0.75f, 0.1f);

	/** 저체력 구간 색상 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Color")
	FLinearColor LowHealthColor = FLinearColor(0.9f, 0.1f, 0.1f);

	/** 피격 플래시 색상 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Damage")
	FLinearColor FlashColor = FLinearColor::White;

	/** 이 비율 이하부터 보간 시작 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Damage", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MidHealthThreshold = 0.5f;

	/** 이 비율 이하부터 고정 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Damage", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LowHealthThreshold = 0.25f;

	/** 피격 플래시 잔상이 사라지는 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Damage")
	float FlashFadeSpeed = 8.0f;

private:
	enum class EBarMode : uint8
	{
		Hidden,
		PlayerAlways,
		EnemyFade
	};

	void BindToASC(UAbilitySystemComponent* InASC);
	void RefreshHealth(bool bShowBar = false);
	void HandlePlayerNameMessage(FGameplayTag Channel, const FGYPlayerNameMessage& Message);

	void TryBindToOwner(AActor* InCharacter);
	/** 메세지 수신 시 호출 */
	void HandleCharacterReadyMessage(FGameplayTag Channel, const FGYCharacterReadyMessage& Message);
	/** 소유하고 있는 액터 찾아오는 함수 */
	AActor* GetOwningActor() const;
	virtual void SetWidgetOwnerActor(AActor* InOwner) override;

	/** 타이머 관리용 */
	void ProcessBindRetry();
	void ProcessFadeOut();
	void StartFadeOutTimer();

	/** 피격 플래시, 저체력 색상 처리용 */
	void TriggerDamageFlash();
	void ProcessColorInterp();
	void UpdateBarColor();
	FLinearColor GetBaseColorForHealth(float HealthPercent) const;

	FTimerHandle BindRetryTimerHandle;
	FTimerHandle FadeDelayTimerHandle;
	FTimerHandle FadeOutTimerHandle;
	FTimerHandle ColorInterpTimerHandle;

	float FlashAlpha = 0.f;
	float CurrentHealthPercent = 1.f;

	EBarMode Mode = EBarMode::Hidden;

	TWeakObjectPtr<AActor> StoredOwner;
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	TWeakObjectPtr<APlayerState> TargetPS;

	float CurrentAlpha = 0.f;

	/** 바인딩 재시도 */
	UPROPERTY(EditDefaultsOnly, Category="GY|UI")
	float BindRetryInterval = 0.25f;

	int32 BindRetryCount = 0;
	const int32 MaxBindRetries = 20;
};
