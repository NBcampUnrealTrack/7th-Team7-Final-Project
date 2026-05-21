#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EnchantCostRow.generated.h"

USTRUCT(BlueprintType)
struct GY_API FEnchantCostRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Currency"))
	FGameplayTag CurrencyTag;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 Amount = 0;
};
