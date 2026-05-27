#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/Interactable.h"
#include "InteractionComponent.generated.h"

// Trace GA(스캔)와 Interact GA(실행) 간 공유 상태.
// 서버: CurrentInteractable / 클라: CurrentOptions
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GY_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void SetCurrentInteractable(const TScriptInterface<IInteractable>& In) { CurrentInteractable = In; }
	TScriptInterface<IInteractable> GetCurrentInteractable() const { return CurrentInteractable; }

	void SetCurrentOptions(const TArray<FInteractionOption>& In) { CurrentOptions = In; }
	const TArray<FInteractionOption>& GetCurrentOptions() const { return CurrentOptions; }

private:
	UPROPERTY()
	TScriptInterface<IInteractable> CurrentInteractable;

	UPROPERTY()
	TArray<FInteractionOption> CurrentOptions;
};
