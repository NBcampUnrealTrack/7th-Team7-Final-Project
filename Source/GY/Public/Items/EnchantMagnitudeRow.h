#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EnchantMagnitudeRow.generated.h"

USTRUCT(BlueprintType)
struct GY_API FEnchantMagnitudeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName OptionId;

	UPROPERTY(EditAnywhere, meta = (Categories = "Enchant.Magnitude"))
	FGameplayTag MagnitudeTag;

	UPROPERTY(EditAnywhere)
	float Min = 0.f;

	UPROPERTY(EditAnywhere)
	float Max = 0.f;
};
