#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "GYDodgeFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYDodgeData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AppliedTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
	float DodgeApplyTime = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
	float MaxDodgeTime = 1.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYDodgeFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYDodgeFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TMap<FGameplayTag, FGYDodgeData> DodgeDataSets;

	const FGYDodgeData* GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const;
};
