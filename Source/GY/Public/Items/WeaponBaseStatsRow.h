#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponBaseStatsRow.generated.h"

class UCurveTable;

USTRUCT(BlueprintType)
struct GY_API FWeaponBaseStatsRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float BaseATK = 0.f;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UCurveTable> EnhanceCurve;
};
