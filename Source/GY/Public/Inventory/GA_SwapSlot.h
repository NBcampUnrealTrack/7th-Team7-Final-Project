// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_SwapSlot.generated.h"

class IItemContainer;

UCLASS()
class GY_API UItemSwapPayload : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TScriptInterface<IItemContainer> FromContainer;;
	UPROPERTY() FGuid FromSlot;
	UPROPERTY() TScriptInterface<IItemContainer> ToContainer;;
	UPROPERTY() FGuid ToSlot;
};

/**
 *
 */
UCLASS()
class GY_API UGA_SwapSlot : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_SwapSlot(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

};
