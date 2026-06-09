#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "AttributeSet.h"
#include "GYSDisableLogic.generated.h"

struct FGYSDisableMontageSet;

UCLASS()
class GY_API UGYSDisableLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	void PlayLoopMontage();
	void OnDisableTimerFired();
	void RemoveInputBlockTag();
	void LockTick();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const FGYSDisableMontageSet* CachedMontageSet = nullptr;
	FGameplayTag CachedInputBlockTag;
	TArray<TTuple<FGameplayAttribute, float>> CachedLockedValues;

	FTimerHandle StartMontageTimer;
	FTimerHandle DisableTimer;
	FTimerHandle EndMontageTimer;
	FTimerHandle LockTimer;

	static constexpr float LockInterval = 0.05f;
};
