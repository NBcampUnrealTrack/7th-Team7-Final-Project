#pragma once

#include "CoreMinimal.h"
#include "Items/ItemFragment.h"
#include "ItemFragment_Enhanceable.generated.h"

class UCurveTable;

UCLASS()
class GY_API UItemFragment_Enhanceable : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UCurveTable> EnhanceCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 MaxLevel = 5;
};
