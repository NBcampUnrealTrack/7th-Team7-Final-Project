#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "GYPeriodicMagnitudeCalculation.generated.h"

UCLASS(Abstract)
class GY_API UGYPercentOfMaxMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY")
	float RatePercentPerSecond = 0.1f;

protected:
	float CalculateFromCaptured(const FGameplayEffectAttributeCaptureDefinition& CaptureDef, const FGameplayEffectSpec& Spec) const;
};

UCLASS()
class GY_API UGYStaminaRegenMagnitude : public UGYPercentOfMaxMagnitude
{
	GENERATED_BODY()
public:
	UGYStaminaRegenMagnitude();
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
private:
	FGameplayEffectAttributeCaptureDefinition MaxStaminaDef;
	FGameplayEffectAttributeCaptureDefinition StaminaRegenRateDef;
};
UCLASS()
class GY_API UGYStaggerRegenMagnitude : public UGYPercentOfMaxMagnitude
{
	GENERATED_BODY()
public:
	UGYStaggerRegenMagnitude();
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
private:
	FGameplayEffectAttributeCaptureDefinition MaxStaggerDef;
};
UCLASS()
class GY_API UGYStunRegenMagnitude : public UGYPercentOfMaxMagnitude
{
	GENERATED_BODY()
public:
	UGYStunRegenMagnitude();
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
private:
	FGameplayEffectAttributeCaptureDefinition MaxStunDef;
};

