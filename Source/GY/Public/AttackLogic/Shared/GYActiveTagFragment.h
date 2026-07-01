#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYActiveTagFragment.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYActiveTagFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYActiveTagFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ActiveTag")
	FGameplayTagContainer Tags;
};
