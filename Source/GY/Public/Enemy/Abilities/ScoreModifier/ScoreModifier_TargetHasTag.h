// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "ScoreModifier.h"
#include "ScoreModifier_TargetHasTag.generated.h"

/**
 *
 */
UCLASS(meta=(DisplayName="Target Has Tag"))
class GY_API UScoreModifier_TargetHasTag : public UScoreModifier
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) FGameplayTagContainer RequiredTags;
	UPROPERTY(EditAnywhere) float BonusScore = 5.f;

	virtual float Evaluate(const UAbilitySystemComponent*,
						   AActor*, AActor* Target) const override
	{
		if (!Target) return 0.f;
		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC) return 0.f;

		FGameplayTagContainer TargetTags;
		TargetASC->GetOwnedGameplayTags(TargetTags);

		const bool bMatches = TargetTags.HasAll(RequiredTags);
		return bMatches ? BonusScore : 0.f;
	}

};
