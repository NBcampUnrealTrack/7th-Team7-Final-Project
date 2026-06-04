// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_UseConsumable.generated.h"


/**
 *
 */
UCLASS()
class GY_API UGA_UseConsumable : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_UseConsumable(const FObjectInitializer&);

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
