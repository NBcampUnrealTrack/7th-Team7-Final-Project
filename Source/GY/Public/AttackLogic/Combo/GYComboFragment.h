#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Shared/GYAttributeCost.h"
#include "GameplayTagContainer.h"
#include "GYComboFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYComboStepData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGYAttributeCost StaminaCost;
};

USTRUCT(BlueprintType)
struct GY_API FGYComboStepSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGYComboStepData> Steps;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYComboFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYComboFragment();

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float ComboCount = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TMap<FGameplayTag, FGYComboStepSet> ComboSteps;

	const TArray<FGYComboStepData>* GetBestMatchingSteps(const FGameplayTagContainer& OwnedTags) const;
};
