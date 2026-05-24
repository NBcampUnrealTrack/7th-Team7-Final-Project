#pragma once

#include "CoreMinimal.h"
#include "AbilityTask_WaitForInteractableTargets.h"
#include "AbilityTask_WaitForInteractableTargets_SphereOverlap.generated.h"

/**
 *
 */
UCLASS()
class GY_API UAbilityTask_WaitForInteractableTargets_SphereOverlap : public UAbilityTask_WaitForInteractableTargets
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks")
	static UAbilityTask_WaitForInteractableTargets_SphereOverlap* WaitForInteractableTargets_SphereOverlap(
		UGameplayAbility* OwningAbility,
		float InteractionScanRange = 300.f,
		float InteractionScanRate  = 0.1f);

	virtual void Activate() override;

private:
	virtual void OnDestroy(bool AbilityEnded) override;

	void PerformOverlap();

	float InteractionScanRange = 300.f;
	float InteractionScanRate  = 0.1f;

	FTimerHandle TimerHandle;
};
