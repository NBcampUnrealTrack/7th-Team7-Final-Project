#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GYDodgeInputLogic.generated.h"

UCLASS()
class GY_API UGYDodgeInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	UFUNCTION()
	void OnIFrameFinished();

	UFUNCTION()
	void OnDodgeEndFinished();

	void RemoveDodgeTag();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	FGameplayTag CachedDodgeAppliedTag;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> IFrameTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> DodgeEndTask;
};
