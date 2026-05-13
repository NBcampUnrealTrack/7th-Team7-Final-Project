#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interaction/InteractionOption.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class GY_API IInteractable
{
	GENERATED_BODY()

public:
	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& Out) const = 0;

	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) = 0;
};
