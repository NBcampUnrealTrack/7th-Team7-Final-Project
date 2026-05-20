// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LevelPlacedActorData.generated.h"
/**
 *
 */
USTRUCT(BlueprintType)
struct GY_API FActorGuidTableRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid ActorGuid;

};
