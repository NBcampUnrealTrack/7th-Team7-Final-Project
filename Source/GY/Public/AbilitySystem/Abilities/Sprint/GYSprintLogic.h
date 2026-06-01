// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYSprintLogic.generated.h"

class UGYSprintFragment;
/**
 *
 */
UCLASS()
class GY_API UGYSprintLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual void OnInputPressed() override;
	virtual void OnInputReleased() override;

	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;
private:

	void CheckStamina();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const UGYSprintFragment* CachedFragment = nullptr;

	FActiveGameplayEffectHandle DrainHandle;
	FTimerHandle StaminaCheckTimer;
};


