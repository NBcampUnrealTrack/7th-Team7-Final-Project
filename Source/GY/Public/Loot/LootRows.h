#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LootRows.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct GY_API FItemPoolRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UItemDefinition> Definition;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 MaxCount = 1;
};

USTRUCT(BlueprintType)
struct GY_API FGradeDistributionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Item.Grade"))
	FGameplayTag GradeTag;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Weight = 1;
};

USTRUCT(BlueprintType)
struct GY_API FLevelDistributionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Level = 1;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Weight = 1;
};
