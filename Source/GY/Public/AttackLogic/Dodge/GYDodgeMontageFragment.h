#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "GYDodgeMontageFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYDodgeMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> DodgeMontage;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYDodgeMontageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYDodgeMontageFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TMap<FGameplayTag, FGYDodgeMontageSet> MontageAnimSets;

	const FGYDodgeMontageSet* GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const;
};
