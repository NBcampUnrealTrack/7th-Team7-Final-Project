#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttackLogic/Shared/GYAttributeCost.h"
#include "GameplayTagContainer.h"
#include "GYDodgeFragment.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYDodgeData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	FGYAttributeCost StaminaCost;

	// How long the DodgeAppliedTag stays on the ASC (invincibility window). Shorter than the full animation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
	float InvincibilityDuration = 0.3f;

	//회피거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DodgeImpulse = 800.f;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYDodgeFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYDodgeFragment();

	// Tag applied to the ASC during the invincibility window. Same tag for all weapon types.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	FGameplayTag DodgeAppliedTag;



	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TMap<FGameplayTag, FGYDodgeData> DodgeDataSets;

	const FGYDodgeData* GetBestMatchingData(const FGameplayTagContainer& OwnedTags) const;
};
