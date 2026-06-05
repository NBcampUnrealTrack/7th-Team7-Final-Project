#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
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
	void OnParryWindowExpired();
	void OnParryAnimExpired();
	void PlayEndMontage();
	void RemoveParryTag();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const FGYParryMontageSet* CachedMontageSet = nullptr;
	const FGYParryData* CachedParryData = nullptr;
	FTimerHandle ParryWindowTimer;
	FTimerHandle ParryAnimTimer;
	FTimerHandle CounterMontageTimer;
	FTimerHandle EndMontageTimer;
};
