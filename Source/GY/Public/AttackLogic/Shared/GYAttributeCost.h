#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GYAttributeCost.generated.h"

USTRUCT(BlueprintType)
struct GY_API FGYAttributeCost
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayAttribute Attribute;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float Amount = 0.f;
};
