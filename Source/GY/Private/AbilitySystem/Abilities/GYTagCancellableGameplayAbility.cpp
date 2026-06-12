// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GYTagCancellableGameplayAbility.h"

#include "AbilitySystemComponent.h"

bool UGYTagCancellableGameplayAbility::DoesAbilitySatisfyTagRequirements(
	const UAbilitySystemComponent& AbilitySystemComponent,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (ActivationTagExceptions.IsEmpty())
	{
		return Super::DoesAbilitySatisfyTagRequirements(AbilitySystemComponent, SourceTags, TargetTags, OptionalRelevantTags);
	}

	FGameplayTagContainer OwnedTags;
	AbilitySystemComponent.GetOwnedGameplayTags(OwnedTags);

	FGameplayTagContainer TagsToIgnore;
	for (const FGYActivationTagException& Exception : ActivationTagExceptions)
	{
		if (Exception.ExceptionTag.IsValid() && OwnedTags.HasTag(Exception.ExceptionTag))
		{
			TagsToIgnore.AppendTags(Exception.IgnoredBlockedTags);
		}
	}

	if (TagsToIgnore.IsEmpty())
	{
		return Super::DoesAbilitySatisfyTagRequirements(AbilitySystemComponent, SourceTags, TargetTags, OptionalRelevantTags);
	}

	FGameplayTagContainer EffectiveBlockedAbilityTags = AbilitySystemComponent.GetBlockedAbilityTags();
	EffectiveBlockedAbilityTags.RemoveTags(TagsToIgnore);

	FGameplayTagContainer EffectiveActivationBlockedTags = ActivationBlockedTags;
	EffectiveActivationBlockedTags.RemoveTags(TagsToIgnore);

	const bool bBlocked = GetAssetTags().HasAny(EffectiveBlockedAbilityTags)
		|| OwnedTags.HasAny(EffectiveActivationBlockedTags);
	const bool bMissing = !OwnedTags.HasAll(ActivationRequiredTags);

	return !bBlocked && !bMissing;
}
