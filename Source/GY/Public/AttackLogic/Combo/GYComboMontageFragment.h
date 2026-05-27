#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "GYComboMontageFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYComboMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TObjectPtr<UAnimMontage>> Montages;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYComboMontageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYComboMontageFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
	TMap<FGameplayTag, FGYComboMontageSet> MontageAnimSets;

	const TArray<TObjectPtr<UAnimMontage>>* GetBestMatchingMontages(const FGameplayTagContainer& OwnedTags) const;
};
