#include "AbilitySystem/Executions/GYDamageExecution.h"

#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
#include "Core/GameplayTags/OptionTags.h"

namespace
{
	struct FGYDamageCapture
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(Attack);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
		DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalRate);
		DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalMultiplier);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Strength);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Dexterity);

		FGYDamageCapture()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UGYDamageAttributeSet, Attack, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UGYDamageAttributeSet, Defense, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UGYDamageAttributeSet, CriticalRate, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UGYDamageAttributeSet, CriticalMultiplier, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UGYCoreStatAttributeSet, Strength, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UGYCoreStatAttributeSet, Dexterity, Source, true);
		}
	};

	static const FGYDamageCapture& DamageCapture()
	{
		static FGYDamageCapture Capture;
		return Capture;
	}

	// 1차 스탯(STR/DEX) 1포인트당 무기 데미지 +1.5%
	constexpr float StatToWeaponDamage = 0.015f;
}

UGYDamageExecution::UGYDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageCapture().AttackDef);
	RelevantAttributesToCapture.Add(DamageCapture().DefenseDef);
	RelevantAttributesToCapture.Add(DamageCapture().CriticalRateDef);
	RelevantAttributesToCapture.Add(DamageCapture().CriticalMultiplierDef);
	RelevantAttributesToCapture.Add(DamageCapture().StrengthDef);
	RelevantAttributesToCapture.Add(DamageCapture().DexterityDef);
}

void UGYDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float Attack = 0.f;
	float Defense = 0.f;
	float CriticalRate = 0.f;
	float CriticalMultiplier = 1.f;
	float Strength = 0.f;
	float Dexterity = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageCapture().AttackDef, EvalParams, Attack);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageCapture().DefenseDef, EvalParams, Defense);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageCapture().CriticalRateDef, EvalParams, CriticalRate);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageCapture().CriticalMultiplierDef, EvalParams, CriticalMultiplier);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageCapture().StrengthDef, EvalParams, Strength);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageCapture().DexterityDef, EvalParams, Dexterity);

	const float Motion = Spec.GetSetByCallerMagnitude(GYGameplayTags::Damage_SetByCaller_MotionMultiplier, false, 1.f);

	float Damage = Attack * Motion * (1.f + (Strength + Dexterity) * StatToWeaponDamage);
	if (FMath::FRand() < CriticalRate)
	{
		Damage *= CriticalMultiplier;
	}

	const float FinalDamage = FMath::Max(0.f, Damage - Defense);
	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UGYVitalAttributeSet::GetCurrentHealthAttribute(), EGameplayModOp::Additive, -FinalDamage));
	}
}
