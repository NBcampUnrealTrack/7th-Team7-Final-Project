#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GameplayTagContainer.h"
#include "GYActiveTagLogic.generated.h"

UCLASS()
class GY_API UGYActiveTagLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;

	// OnExecute에서 실제로 부여한 태그 — OnAbilityEnd에서 정확히 이 값만 제거
	FGameplayTagContainer AppliedTags;
};
