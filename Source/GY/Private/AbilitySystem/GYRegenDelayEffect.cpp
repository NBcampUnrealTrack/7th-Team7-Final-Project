#include "AbilitySystem/GYRegenDelayEffect.h"

UGYRegenDelayEffect::UGYRegenDelayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));
}
