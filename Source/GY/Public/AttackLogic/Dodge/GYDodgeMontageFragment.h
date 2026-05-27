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

USTRUCT(BlueprintType)
struct GY_API FGYDodgeWeaponMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FGYDodgeMontageSet> DirectionMontages;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYDodgeMontageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYDodgeMontageFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TMap<FGameplayTag, FGYDodgeWeaponMontageSet> MontageAnimSets;

	const FGYDodgeMontageSet* GetMontageForWeaponAndDirection(
		const FGameplayTagContainer& OwnedTags,
		FGameplayTag DirectionTag) const;
};
