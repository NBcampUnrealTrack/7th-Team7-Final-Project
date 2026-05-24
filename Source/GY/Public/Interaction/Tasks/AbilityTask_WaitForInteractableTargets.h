#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitForInteractableTargets.generated.h"

class IInteractable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FInteractableObjectsChangedEvent,
	const TScriptInterface<IInteractable>&, Interactables);

/**
 *
 */
UCLASS()
class GY_API UAbilityTask_WaitForInteractableTargets : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FInteractableObjectsChangedEvent InteractableObjectsChanged;


protected:
	void UpdateInteractableOptions(
		APawn* Interactor,
		const TScriptInterface<IInteractable>& Interactable);

	TScriptInterface<IInteractable> CurrentInteractable;

};
