#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYChargeSuperArmorFragment.generated.h"

//   ___ _____ _   _ ___
//  / __|_   _| | | | _ )
//  \__ \ | | | |_| | _ \
//  |___/ |_|  \___/|___/

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYChargeSuperArmorFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYChargeSuperArmorFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SuperArmor")
	FGameplayTagContainer RequiredTags;

	bool IsActive(const FGameplayTagContainer& OwnedTags) const;
};
