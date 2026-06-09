#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "GYSDisableMontageFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYSDisableMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> StartMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> LoopMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName LoopSection = TEXT("Loop");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> EndMontage;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYSDisableMontageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYSDisableMontageFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SDisable")
	TMap<FGameplayTag, FGYSDisableMontageSet> MontageAnimSets;

	const FGYSDisableMontageSet* GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const;
};
