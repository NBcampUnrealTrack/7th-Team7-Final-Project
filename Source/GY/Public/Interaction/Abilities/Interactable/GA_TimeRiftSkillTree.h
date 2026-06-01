// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_TimeRiftSkillTree.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGA_TimeRiftSkillTree : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TimeRiftSkillTree(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnExitEventReceived(FGameplayEventData Payload);
};
