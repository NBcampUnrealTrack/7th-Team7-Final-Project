#include "AbilitySystem/GYPeriodicAttributeEffect.h"

UGYPeriodicAttributeEffect::UGYPeriodicAttributeEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period.Value = 0.1f;
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo ModInfo;
	ModInfo.ModifierOp = EGameplayModOp::AddFinal;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FCustomCalculationBasedFloat());
	Modifiers.Add(ModInfo);
}
