#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Shared/GYAttributeCost.h"
#include "GameplayTagContainer.h"
#include "GYBlockFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYBlockData
{
	GENERATED_BODY()

	// Stamina drained per second while blocking.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block")
	FGYAttributeCost StaminaDrainPerSecond;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYBlockFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYBlockFragment();

	// Tag applied to the ASC for the full duration of the block. Same for all weapon types.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block")
	FGameplayTag BlockAppliedTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block")
	TMap<FGameplayTag, FGYBlockData> BlockDataSets;

	const FGYBlockData* GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const;
};
