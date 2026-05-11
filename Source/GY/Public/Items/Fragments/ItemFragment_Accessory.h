#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Accessory.generated.h"

UCLASS()
class GY_API UItemFragment_Accessory : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AccessoryTypeTag;
};
