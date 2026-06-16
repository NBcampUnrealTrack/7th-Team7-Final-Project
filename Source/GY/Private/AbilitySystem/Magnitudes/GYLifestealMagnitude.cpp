#include "AbilitySystem/Magnitudes/GYLifestealMagnitude.h"

#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "Core/GameplayTags/OptionTags.h"

UGYLifestealMagnitude::UGYLifestealMagnitude()
{
	// 흡혈은 공격자 자신에게 적용되므로 Target 캡처 = 공격자 MaxHealth.
	MaxHealthDef.AttributeToCapture = UGYVitalAttributeSet::GetMaxHealthAttribute();
	MaxHealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxHealthDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(MaxHealthDef);
}

float UGYLifestealMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxHealth = 0.f;
	GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvalParams, MaxHealth);

	// 롤값은 퍼센트(예: 1.5 = 1.5%)로 저장 → 비율로 환산해 최대 체력에 곱.
	constexpr float PercentToRatio = 0.01f;
	const float Percent = Spec.GetSetByCallerMagnitude(GYGameplayTags::Stat_Modifier_OptionMagnitude1, false, 0.f);
	return MaxHealth * Percent * PercentToRatio;
}
