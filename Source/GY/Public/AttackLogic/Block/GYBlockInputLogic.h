#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYBlockInputLogic.generated.h"

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
	void DrainTick();
	void PlayBlockEnd();
	void RemoveBlockTag();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const struct FGYBlockMontageSet* CachedMontageSet = nullptr;
	FGameplayTag CachedBlockAppliedTag;
	FGameplayAttribute CachedDrainAttribute;
	float CachedDrainPerSecond = 0.f;
	bool bEnding = false;
	FTimerHandle DrainTimer;
	FTimerHandle EndMontageTimer;

	static constexpr float DrainInterval = 0.1f;
};
