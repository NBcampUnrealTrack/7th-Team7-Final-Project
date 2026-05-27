#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_TraceInteraction.generated.h"

class IInteractable;

// OnSpawn passive. 근처 Interactable을 스캔해서 Character의 UInteractionComponent에 기록.
// 클라: 옵션(UI) / 서버: CurrentInteractable. 실행은 GA_Interact가 담당.
UCLASS()
class GY_API UGA_TraceInteraction : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TraceInteraction(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnOptionsUpdated(const TScriptInterface<IInteractable>& Interactable);

	UFUNCTION()
	void OnNearestInteractableChanged(const TScriptInterface<IInteractable>& Interactable);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionScanRange = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionScanRate = 0.1f;
};
