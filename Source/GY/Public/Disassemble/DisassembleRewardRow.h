#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "DisassembleRewardRow.generated.h"

USTRUCT(BlueprintType)
struct GY_API FDisassembleRewardRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Item.Grade"))
	FGameplayTag GradeTag;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Level = 1;

	UPROPERTY(EditAnywhere, meta = (Categories = "Currency"))
	FGameplayTag CurrencyTag;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 Amount = 0;
};
