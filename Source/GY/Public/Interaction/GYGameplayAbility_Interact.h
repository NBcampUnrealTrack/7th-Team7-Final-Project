// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GYGameplayAbility_Interact.generated.h"

class IInteractable;
struct FInteractionOption;
/**
 *
 */
UCLASS()
class GY_API UGYGameplayAbility_Interact : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGYGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void UpdateInteraction(const TScriptInterface<IInteractable>& Interactable);

	void TriggerInteraction();


protected:
	TScriptInterface<IInteractable> CurrentInteractable;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionScanRange = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionScanRate = 0.1f;
};
