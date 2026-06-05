#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "GYBlockMontageFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYBlockMontageSet
{
	GENERATED_BODY()

	// Played when block starts. Should have a looping section to hold indefinitely.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> HoldMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName HoldLoopSection = TEXT("Loop");

	// Played when block ends before the ability fully ends.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> EndMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> BlockBreakMontage;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYBlockMontageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYBlockMontageFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block")
	TMap<FGameplayTag, FGYBlockMontageSet> MontageAnimSets;

	const FGYBlockMontageSet* GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const;
};
