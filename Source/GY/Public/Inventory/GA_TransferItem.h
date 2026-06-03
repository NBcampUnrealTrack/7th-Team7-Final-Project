// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_TransferItem.generated.h"

class IItemContainer;

UCLASS()
class GY_API UItemTransferPayload : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TScriptInterface<IItemContainer> FromContainer;;
	UPROPERTY() FGuid FromInstanceId;
	UPROPERTY() TScriptInterface<IItemContainer> ToContainer;;
};

/**
 *
 */
UCLASS()
class GY_API UGA_TransferItem : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_TransferItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

};
