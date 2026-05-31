#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Core/GYUserWidget.h"
#include "UI/GYCharacterBoundUIInterface.h"
#include "GYGaugeCircleWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UAbilitySystemComponent;
struct FGYAttributeValueMessage;
/**
 * 원형 게이지바 위젯
 */
UCLASS()
class GYUI_API UGYGaugeCircleWidget : public UGYUserWidget, public IGYCharacterBoundUI
{
	GENERATED_BODY()

public:
	/** Owner가 로컬 플레이어일 때만 ASC 바인딩 */
	virtual void BindToOwnerCharacter(AActor* InCharacter) override;

	void BindToASC(UAbilitySystemComponent* InASC);
	void UnbindFromASC();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnPoiseChanged(const FOnAttributeChangeData& Data);
	void OnStaminaChanged(const FOnAttributeChangeData& Data);

	void RefreshHP();
	void RefreshPoise();
	void RefreshStamina();

	void SetPercent(UImage* Image, float Percent);
	void NotifyActivity();

	struct FAttrBinding
	{
		FGameplayAttribute Attribute;
		FDelegateHandle Handle;
	};
	TArray<FAttrBinding> Bindings;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	float  CurrentAlpha = 0.f;
	double LastActivityTime = -1000.0;
	double BindTime = -1000.0;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, TObjectPtr<UMaterialInstanceDynamic>> MIDCache;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, float> LastPercent;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, float> TargetPercents;
	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, float> CurrentPercents;
};

