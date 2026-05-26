#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYChargeInputLogic.generated.h"

struct FGYChargeMontageSet;

UCLASS()
class GY_API UGYChargeInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	void ExecuteAttack();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	float ChargeStartTime = 0.f;
	bool bCharging = false;
	FTimerHandle MaxChargeTimer;
	FTimerHandle MontageEndTimer;
	const FGYChargeMontageSet* CachedMontageSet = nullptr;
};
