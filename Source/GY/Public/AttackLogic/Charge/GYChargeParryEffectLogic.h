#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYChargeParryEffectLogic.generated.h"

struct FGYChargeParryEffectData;

UCLASS()
class GY_API UGYChargeParryEffectLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	void RemoveAppliedTags();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const FGYChargeParryEffectData* CachedData = nullptr;
	bool bTagsApplied = false;
};
