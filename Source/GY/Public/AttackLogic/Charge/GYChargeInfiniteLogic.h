#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYChargeInfiniteLogic.generated.h"

UCLASS()
class GY_API UGYChargeInfiniteLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;
};
