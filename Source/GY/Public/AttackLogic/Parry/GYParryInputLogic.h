#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GYParryInputLogic.generated.h"

struct FGYParryMontageSet;
struct FGYParryData;

UCLASS()
class GY_API UGYParryInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	UFUNCTION()
	void OnParryWindowExpired();

	UFUNCTION()
	void OnParryAnimExpired();

	UFUNCTION()
	void OnCounterMontageFinished();

	UFUNCTION()
	void OnEndMontageFinished();

	void PlayEndMontage();
	void RemoveParryTag();
	void CancelPendingTasks();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const FGYParryMontageSet* CachedMontageSet = nullptr;
	const FGYParryData* CachedParryData = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> ParryWindowTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> ParryAnimTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> CounterMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> EndMontageTask;
};
