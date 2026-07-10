#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYChargeSuperArmorLogic.generated.h"

//   ___ _____ _   _ ___
//  / __|_   _| | | | _ )
//  \__ \ | | | |_| | _ \
//  |___/ |_|  \___/|___/

UCLASS()
class GY_API UGYChargeSuperArmorLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;
};
