#pragma once

#include "CoreMinimal.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_GrantedAbilitySet.generated.h"

class UAbilitySet;

UCLASS()
class GY_API UItemFragment_GrantedAbilitySet : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAbilitySet> AbilitySet;
};
