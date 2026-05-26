#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYCancelFragment.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYCancelFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYCancelFragment();

	UPROPERTY(EditDefaultsOnly, Category = "Cancel")
	FGameplayTagContainer CancelEventTags;

	UPROPERTY(EditDefaultsOnly, Category = "Cancel", meta = (ClampMin = "-1.0"))
	float MontageBlendOutTime = -1.f;
};
