#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ArmorBaseStatsRow.generated.h"

USTRUCT(BlueprintType)
struct GY_API FArmorBaseStatsRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float BaseDEF = 0.f;

	UPROPERTY(EditAnywhere)
	float BaseHP = 0.f;
};
