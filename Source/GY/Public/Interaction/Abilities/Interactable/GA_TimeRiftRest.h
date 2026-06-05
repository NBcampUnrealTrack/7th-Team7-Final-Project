// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_TimeRiftRest.generated.h"

class UItemDefinition;
/**
 *
 */
UCLASS()
class GY_API UGA_TimeRiftRest : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_TimeRiftRest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Rest")
	TSubclassOf<UGameplayEffect> RecoveryEffect;
	UPROPERTY(EditDefaultsOnly, Category = "Rest")
	float RestAdvanceHour;

	UPROPERTY(EditDefaultsOnly, Category = "Rest")
	TArray<TSoftObjectPtr<UItemDefinition>> RefillPotionDefs;

};
