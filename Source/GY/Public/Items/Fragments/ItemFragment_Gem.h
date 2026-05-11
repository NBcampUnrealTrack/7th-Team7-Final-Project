#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Gem.generated.h"

class UAbilitySet;

UCLASS()
class GY_API UItemFragment_Gem : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Gem.Type"))
	FGameplayTag GemTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Weapon.Type"))
	FGameplayTagContainer ApplicableWeaponTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAbilitySet> AbilitySet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0))
	int32 INTCost = 0;
};
