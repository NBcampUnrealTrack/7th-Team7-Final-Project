#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "GYChargeMontageFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYChargeMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ChargeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ChargeLoopSection = TEXT("Loop");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AttackMontage;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYChargeMontageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYChargeMontageFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
	TMap<FGameplayTag, FGYChargeMontageSet> MontageAnimSets;

	const FGYChargeMontageSet* GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const;
};
