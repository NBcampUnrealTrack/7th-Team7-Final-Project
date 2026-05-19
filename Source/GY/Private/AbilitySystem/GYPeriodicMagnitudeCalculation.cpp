#include "AbilitySystem/GYPeriodicMagnitudeCalculation.h"

#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"

float UGYPercentOfMaxMagnitude::CalculateFromCaptured(const FGameplayEffectAttributeCaptureDefinition& CaptureDef, const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxValue = 0.f;
	GetCapturedAttributeMagnitude(CaptureDef, Spec, EvalParams, MaxValue);

	const float Period = Spec.Def ? Spec.Def->Period.GetValueAtLevel(Spec.GetLevel()) : 0.1f;
	return MaxValue * RatePercentPerSecond * Period;
}

UGYStaminaRegenMagnitude::UGYStaminaRegenMagnitude()
{
	MaxStaminaDef.AttributeToCapture = UGYPlayerAttribute::GetMaxStaminaAttribute();
	MaxStaminaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxStaminaDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(MaxStaminaDef);
}

float UGYStaminaRegenMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return CalculateFromCaptured(MaxStaminaDef, Spec);
}

UGYHitResRegenMagnitude::UGYHitResRegenMagnitude()
{
	MaxHitResDef.AttributeToCapture = UGYAdditionalAttribute::GetMaxHitResAttribute();
	MaxHitResDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxHitResDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(MaxHitResDef);
}

float UGYHitResRegenMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return CalculateFromCaptured(MaxHitResDef, Spec);
}

UGYPoiseRegenMagnitude::UGYPoiseRegenMagnitude()
{
	MaxPoiseDef.AttributeToCapture = UGYAdditionalAttribute::GetMaxPoiseAttribute();
	MaxPoiseDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxPoiseDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(MaxPoiseDef);
}

float UGYPoiseRegenMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return CalculateFromCaptured(MaxPoiseDef, Spec);
}
