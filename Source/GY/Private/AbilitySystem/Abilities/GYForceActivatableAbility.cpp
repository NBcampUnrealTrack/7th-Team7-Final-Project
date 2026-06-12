// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GYForceActivatableAbility.h"

#include "AbilitySystemComponent.h"

bool UGYForceActivatableAbility::DoesAbilitySatisfyTagRequirements(
	const UAbilitySystemComponent& AbilitySystemComponent,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	FGameplayTagContainer OwnedTags;
	AbilitySystemComponent.GetOwnedGameplayTags(OwnedTags);

	if (ForceActivateTags.IsEmpty() || !OwnedTags.HasAnyExact(ForceActivateTags))
	{
		return Super::DoesAbilitySatisfyTagRequirements(AbilitySystemComponent, SourceTags, TargetTags, OptionalRelevantTags);
	}

	FGameplayTagContainer EffectiveBlockedAbilityTags = AbilitySystemComponent.GetBlockedAbilityTags();
	EffectiveBlockedAbilityTags.RemoveTags(GetAssetTags());

	FGameplayTagContainer EffectiveActivationBlockedTags = ActivationBlockedTags;
	EffectiveActivationBlockedTags.RemoveTags(GetAssetTags());

	const bool bBlocked = GetAssetTags().HasAny(EffectiveBlockedAbilityTags)
		|| OwnedTags.HasAny(EffectiveActivationBlockedTags);
	const bool bMissing = !OwnedTags.HasAll(ActivationRequiredTags);

	return !bBlocked && !bMissing;
}
