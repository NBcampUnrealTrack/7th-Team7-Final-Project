#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Shared/GYAttributeCost.h"
#include "GameplayTagContainer.h"
#include "GYParryFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYParryData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
	float ParryTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
	float ParryAnimTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	FGYAttributeCost StaminaCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	FGYAttributeCost StaminaReward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry")
	FGameplayTagContainer ParryAppliedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry")
	TArray<FGYAttributeEffect> ReceiverAffected;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYParryFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYParryFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry")
	TMap<FGameplayTag, FGYParryData> ParryDataSets;

	const FGYParryData* GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const;
};
