#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets.h"
#include "AbilitySystemComponent.h"
#include "Interaction/Interactable.h"



void UAbilityTask_WaitForInteractableTargets::UpdateInteractableOptions(
	APawn* Interactor,
	const TScriptInterface<IInteractable>& Interactable)
{
	if (Interactable != CurrentInteractable)
	{
		CurrentInteractable = Interactable;
		InteractableObjectsChanged.Broadcast(CurrentInteractable);
	}
}
