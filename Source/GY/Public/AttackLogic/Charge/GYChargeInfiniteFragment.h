#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYChargeInfiniteFragment.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYChargeInfiniteFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYChargeInfiniteFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InfiniteCharge")
	FGameplayTagContainer RequiredTags;

	bool IsActive(const FGameplayTagContainer& OwnedTags) const;
};
