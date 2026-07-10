#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Shared/GYAttributeCost.h"
#include "GameplayTagContainer.h"
#include "GYChargeParryEffectFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYChargeParryEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer AppliedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	FGYAttributeCost StaminaReward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGYAttributeEffect> ReceiverAffected;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYChargeParryEffectFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYChargeParryEffectFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChargeParryEffect")
	TMap<FGameplayTag, FGYChargeParryEffectData> EffectSets;

	const FGYChargeParryEffectData* GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const;
};
