#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYDodgeInputLogic.generated.h"

struct FGYDodgeData;

UCLASS()
class GY_API UGYDodgeInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	void OnTagWindowExpired();
	void OnAnimationExpired();
	void RemoveAppliedTag();
	void RemoveAnimationTag();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const FGYDodgeData* CachedDodgeData = nullptr;
	FTimerHandle TagTimer;
	FTimerHandle EndTimer;
};
