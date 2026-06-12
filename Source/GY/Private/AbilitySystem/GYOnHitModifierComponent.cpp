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

void UGYOnHitModifierComponent::CollectModifiers(TArray<FRolledMagnitude>& OutModifiers) const
{
	for (const TPair<FObjectKey, TArray<FRolledMagnitude>>& Pair : ModifiersBySource)
	{
		OutModifiers.Append(Pair.Value);
	}
}
