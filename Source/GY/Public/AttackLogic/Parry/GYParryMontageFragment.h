#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "GYParryMontageFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYParryMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ParryMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ParryLoopSection = TEXT("Loop");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> EndMontage;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYParryMontageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYParryMontageFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry")
	TMap<FGameplayTag, FGYParryMontageSet> MontageAnimSets;

	const FGYParryMontageSet* GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const;
};
