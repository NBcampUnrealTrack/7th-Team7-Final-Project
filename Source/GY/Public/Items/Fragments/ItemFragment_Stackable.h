#pragma once

#include "CoreMinimal.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Stackable.generated.h"

UCLASS()
class GY_API UItemFragment_Stackable : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 MaxStackSize = 1;
};
