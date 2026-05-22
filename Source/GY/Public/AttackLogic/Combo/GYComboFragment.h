#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GYComboFragment.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYComboFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYComboFragment();

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float ComboCount = 3.f;
};
