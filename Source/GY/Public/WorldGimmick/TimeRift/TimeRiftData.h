#pragma once

#include "CoreMinimal.h"
#include "TimeRiftData.generated.h"

USTRUCT()
struct GY_API FTimeRiftStaticData
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;
	UPROPERTY()
	FTransform RespawnTransform;
};

USTRUCT()
struct GY_API FTimeRiftDynamicData
{
	GENERATED_BODY()

	//TODO 저장해야함
	UPROPERTY()
	bool bDiscovered = false;
	UPROPERTY()
	bool bUnlocked = true;

};
