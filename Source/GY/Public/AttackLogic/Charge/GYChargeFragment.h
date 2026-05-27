#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Shared/GYAttributeCost.h"
#include "GameplayTagContainer.h"
#include "GYChargeFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYChargeData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
	float MinChargeTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
	float MaxChargeTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1.0"))
	float DamageMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	FGYAttributeCost ChargeCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	FGYAttributeCost AttackCost;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYChargeFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYChargeFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge")
	TMap<FGameplayTag, FGYChargeData> ChargeDataSets;

	const FGYChargeData* GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const;
};
