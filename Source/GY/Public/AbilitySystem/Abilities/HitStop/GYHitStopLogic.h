// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYHitStopLogic.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGYHitStopLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;

	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;

	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;

	virtual void OnGameplayEvent(FGameplayTag Tag, const FGameplayEventData& Payload) override;

	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
};
