#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Material.generated.h"

UCLASS()
class GY_API UItemFragment_Material : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Material.Tier"))
	FGameplayTag MaterialTierTag;
};
