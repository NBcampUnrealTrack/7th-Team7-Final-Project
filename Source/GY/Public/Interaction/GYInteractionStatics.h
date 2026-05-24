#pragma once

#include "CoreMinimal.h"
#include "Interactable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GYInteractionStatics.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGYInteractionStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void AppendInteractablesFromOverlapResults(
		const TArray<FOverlapResult>& OverlapResults,
		TArray<TScriptInterface<IInteractable>>& OutInteractables);

};
