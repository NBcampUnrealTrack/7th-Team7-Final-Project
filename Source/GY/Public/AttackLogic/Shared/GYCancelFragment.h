#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYCancelFragment.generated.h"

USTRUCT(BlueprintType)
struct FGYCancelWindowEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag EventTag;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag WindowTag;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYCancelFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYCancelFragment();

	UPROPERTY(EditDefaultsOnly, Category = "Cancel")
	TArray<FGameplayTag> CancelEvents;

	UPROPERTY(EditDefaultsOnly, Category = "Cancel")
	TArray<FGYCancelWindowEntry> CancelWithinWindow;

	UPROPERTY(EditDefaultsOnly, Category = "Cancel", meta = (ClampMin = "-1.0"))
	float MontageBlendOutTime = -1.f;
};
