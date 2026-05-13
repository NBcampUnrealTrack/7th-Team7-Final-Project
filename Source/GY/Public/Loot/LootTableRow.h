#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LootTableRow.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct GY_API FLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName SourceId;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UItemDefinition> Definition;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 MaxCount = 1;
};
