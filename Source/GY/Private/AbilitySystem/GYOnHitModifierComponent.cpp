#include "AbilitySystem/GYOnHitModifierComponent.h"

void UGYOnHitModifierComponent::RegisterModifiers(const UObject* Source, const TArray<FRolledMagnitude>& Modifiers)
{
	if (!IsValid(Source) || Modifiers.IsEmpty()) return;
	ModifiersBySource.Add(FObjectKey(Source), Modifiers);
}

void UGYOnHitModifierComponent::UnregisterModifiers(const UObject* Source)
{
	if (!IsValid(Source)) return;
	ModifiersBySource.Remove(FObjectKey(Source));
}

float UGYOnHitModifierComponent::GetModifierSumValue(FGameplayTag MagnitudeTag) const
{
	float Sum = 0.f;
	for (const TPair<FObjectKey, TArray<FRolledMagnitude>>& Pair : ModifiersBySource)
	{
		for (const FRolledMagnitude& Modifier : Pair.Value)
		{
			if (Modifier.MagnitudeTag == MagnitudeTag)
			{
				Sum += Modifier.Value;
			}
		}
	}
	return Sum;
}
