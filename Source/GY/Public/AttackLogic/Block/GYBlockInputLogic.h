#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameplayEffectTypes.h"
#include "GYBlockInputLogic.generated.h"

struct FOnAttributeChangeData;

UCLASS()
class GY_API UGYBlockInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual void OnInputReleased() override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	void ApplyDrainEffect();
	void RemoveDrainEffect();
	void PlayBlockEnd();
	void PlayBlockBreak();
	void RemoveBlockTag();

	void OnStaminaChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void OnBlockEndMontageFinished();

	UFUNCTION()
	void OnBlockBreakMontageFinished();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const struct FGYBlockMontageSet* CachedMontageSet = nullptr;
	FGameplayTag CachedBlockAppliedTag;
	FGameplayAttribute CachedDrainAttribute;
	float CachedDrainPerSecond = 0.f;
	FGameplayAttribute CachedHitCostAttribute;
	float CachedHitCostMultiplier = 1.f;
	bool bEnding = false;

	FActiveGameplayEffectHandle DrainEffectHandle;
	FDelegateHandle StaminaDelegateHandle;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> EndMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> BlockBreakTask;
};
