#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYDodgeInputLogic.generated.h"

struct FGYDodgeData;
struct FGYDodgeMontageSet;

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
	void OnMontageExpired();
	void RemoveAppliedTag();

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const FGYDodgeMontageSet* CachedMontageSet = nullptr;
	const FGYDodgeData* CachedDodgeData = nullptr;
	FTimerHandle TagTimer;
	FTimerHandle EndTimer;
};
