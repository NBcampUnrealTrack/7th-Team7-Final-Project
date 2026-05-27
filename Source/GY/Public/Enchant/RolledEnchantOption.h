#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RolledEnchantOption.generated.h"

USTRUCT(BlueprintType)
struct GY_API FRolledMagnitude
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag MagnitudeTag;

	UPROPERTY(BlueprintReadOnly)
	float Value = 0.f;
};

USTRUCT(BlueprintType)
struct GY_API FRolledEnchantOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName OptionId;

	UPROPERTY(BlueprintReadOnly)
	TArray<FRolledMagnitude> Magnitudes;
};
