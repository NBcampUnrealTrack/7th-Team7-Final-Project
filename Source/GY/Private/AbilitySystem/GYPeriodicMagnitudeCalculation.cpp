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

	CurrentStaminaDef.AttributeToCapture = UGYPlayerAttribute::GetCurrentStaminaAttribute();
	CurrentStaminaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	CurrentStaminaDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(CurrentStaminaDef);
}

float UGYStaminaRegenMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxStamina = 0.f;
	GetCapturedAttributeMagnitude(MaxStaminaDef, Spec, EvalParams, MaxStamina);

	float CurrentStamina = 0.f;
	GetCapturedAttributeMagnitude(CurrentStaminaDef, Spec, EvalParams, CurrentStamina);

	const float Rate = CurrentStamina < 0.f ? NegativeRatePercentPerSecond : RatePercentPerSecond;
	const float Period = Spec.Def ? Spec.Def->Period.GetValueAtLevel(Spec.GetLevel()) : 0.1f;
	return MaxStamina * Rate * Period;
}

UGYStaggerRegenMagnitude::UGYStaggerRegenMagnitude()
{
	MaxStaggerDef.AttributeToCapture = UGYAdditionalAttribute::GetMaxStaggerAttribute();
	MaxStaggerDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxStaggerDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(MaxStaggerDef);
}

float UGYStaggerRegenMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return CalculateFromCaptured(MaxStaggerDef, Spec);
}

UGYStunRegenMagnitude::UGYStunRegenMagnitude()
{
	MaxStunDef.AttributeToCapture = UGYAdditionalAttribute::GetMaxStunAttribute();
	MaxStunDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxStunDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(MaxStunDef);
}

float UGYStunRegenMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return CalculateFromCaptured(MaxStunDef, Spec);
}
