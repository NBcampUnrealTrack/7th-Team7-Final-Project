// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LadderTypeDataTableRow.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct GY_API FLadderTypeDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	TObjectPtr<class UStaticMesh> Rungs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	TObjectPtr<class UStaticMesh> SideRungs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	TObjectPtr<class UStaticMesh> TopLadder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	TObjectPtr<class UStaticMesh> WallConnection;


};


