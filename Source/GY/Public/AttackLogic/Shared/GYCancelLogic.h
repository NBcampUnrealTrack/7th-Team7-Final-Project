#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GameplayTagContainer.h"
#include "GYCancelLogic.generated.h"

class UGYCancelFragment;
class UAbilitySystemComponent;

UCLASS()
class GY_API UGYCancelLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

private:
	void DoCancel(UAbilitySystemComponent* ASC, const UGYCancelFragment* Fragment);

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
};
