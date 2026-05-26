// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Interaction/Interactable.h"
#include "GYAbilityTask_GrantNearbyInteraction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGYNearestInteractableChangedEvent,
	const TScriptInterface<IInteractable>&, Interactable);
/**
 *
 */
UCLASS()
class GY_API UGYAbilityTask_GrantNearbyInteraction : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FGYNearestInteractableChangedEvent NearestInteractableChanged;

	static UGYAbilityTask_GrantNearbyInteraction* GrantAbilitiesForNearbyInteractors(
		UGameplayAbility* OwningAbility, float InScanRange, float InScanRate);

	virtual void Activate() override;

private:
	virtual void OnDestroy(bool AbilityEnded) override;

	void QueryInteractables();

	float ScanRange = 500.f;
	float ScanRate  = 0.1f;

	FTimerHandle TimerHandle;
	TMap<FObjectKey, FGameplayAbilitySpecHandle> AbilityCache;
	TScriptInterface<IInteractable> CurrentNearest;
};
