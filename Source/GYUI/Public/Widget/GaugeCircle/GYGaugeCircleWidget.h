#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Core/GYUserWidget.h"
#include "GYGaugeCircleWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UAbilitySystemComponent;
struct FGYCharacterReadyMessage;
/**
 * 원형 게이지바 위젯
 */
UCLASS()
class GYUI_API UGYGaugeCircleWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
	void BindToASC(UAbilitySystemComponent* InASC);
	void UnbindFromASC();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_HP;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_Poise;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_SP;

	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	FName PercentParamName = TEXT("Percent");

	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	float HoldDuration = 1.5f;
	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	float FadeSpeed = 6.f;
	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	float BindGracePeriod = 0.5f;
	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	float InterpSpeed = 5.f;

private:
	/** GMS 메세지 기반 바인딩 */
	void TryBindToOwner(AActor* InCharacter);
	void HandleCharacterReadyMessage(FGameplayTag Channel, const FGYCharacterReadyMessage& Message);
	AActor* GetOwningActor() const;

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnPoiseChanged(const FOnAttributeChangeData& Data);
	void OnStaminaChanged(const FOnAttributeChangeData& Data);

	void RefreshHP(bool bFromGameplay = false);
	void RefreshPoise(bool bFromGameplay = false);
	void RefreshStamina(bool bFromGameplay = false);

	void SetPercent(UImage* Image, float Percent, bool bFromGameplay);
	void NotifyActivity();
	virtual void SetWidgetOwnerActor(AActor* InOwner) override;
	bool IsLocalPlayerPawn() const;

	/** 타이머 관리용 */
	void ProcessBindRetry();
	void ProcessVisualInterpolation();
	void StartFadeOutTimer();
	void EnsureInterpolationRunning();

	FTimerHandle BindRetryTimerHandle;
	FTimerHandle InterpolationTimerHandle;
	FTimerHandle HoldDelayTimerHandle;

	struct FAttrBinding
	{
		FGameplayAttribute Attribute;
		FDelegateHandle Handle;
	};
	TArray<FAttrBinding> Bindings;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	float  CurrentAlpha = 0.f;
	float  TargetAlpha = 0.f;
	double BindTime = -1000.0;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, TObjectPtr<UMaterialInstanceDynamic>> MIDCache;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, float> TargetPercents;
	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, float> CurrentPercents;

	/** 바인딩 재시도 설정 */
	UPROPERTY(EditDefaultsOnly, Category="GY|Radial")
	float BindRetryInterval = 0.25f;

	int32 BindRetryCount = 0;
	const int32 MaxBindRetries = 20;
};

