#include "Interaction/GYInteractionStatics.h"
#include "Engine/OverlapResult.h"

void UGYInteractionStatics::AppendInteractablesFromOverlapResults(
	const TArray<FOverlapResult>& OverlapResults,
	TArray<TScriptInterface<IInteractable>>& OutInteractables)
{
	for (const FOverlapResult& Overlap : OverlapResults)
	{
		TScriptInterface<IInteractable> Target(Overlap.GetComponent());
		if (Target)
		{
			OutInteractables.AddUnique(Target);
			continue;
		}

		Target = TScriptInterface<IInteractable>(Overlap.GetActor());
		if (Target)
		{
			OutInteractables.AddUnique(Target);
		}
	}
}
