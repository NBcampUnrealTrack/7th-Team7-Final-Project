// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "Interaction/InteractionOption.h"
#include "GYGameplayAbility_Interact.generated.h"

class IInteractable;

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

	UFUNCTION()
	void OnNearestInteractableChanged(const TScriptInterface<IInteractable>& Interactable);

	UFUNCTION()
	void OnInteractEventReceived(FGameplayEventData Payload);

	void TriggerInteraction(FGameplayTag OptionTag = FGameplayTag());


protected:
	TScriptInterface<IInteractable> CurrentInteractable;

	UPROPERTY()
	TArray<FInteractionOption> CurrentOptions;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionScanRange = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionScanRate = 0.1f;
};
