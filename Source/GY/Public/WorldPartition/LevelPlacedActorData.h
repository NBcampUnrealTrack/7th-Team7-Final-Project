// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LevelPlacedActorData.generated.h"
/**
 *
 */
USTRUCT(BlueprintType)
struct GY_API FActorRegionTableRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid ActorGuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName RegionName;
};

USTRUCT(BlueprintType)
struct GY_API FRegionDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RegionName;

};
