#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYComboFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYDamageMultiplierSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<float> Multipliers;
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
	TMap<FGameplayTag, FGYDamageMultiplierSet> DamageMultipliers;

	const TArray<float>* GetBestMatchingMultipliers(const FGameplayTagContainer& OwnedTags) const;
};
