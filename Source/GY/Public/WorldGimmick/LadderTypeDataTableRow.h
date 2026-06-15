#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LadderTypeDataTableRow.generated.h"

USTRUCT(BlueprintType)
struct GY_API FLadderTypeDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder")
	TObjectPtr<UStaticMesh> RungMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder")
	TObjectPtr<UStaticMesh> TopGrabBarMesh;
	//사이드바
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder")
	TObjectPtr<UStaticMesh> PoleMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder")
	TObjectPtr<UStaticMesh> WallBracketMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder|Layout")
	float RungSpacing = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder|Layout")
	float LadderWidth = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder|Layout")
	float PoleSegmentLength = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder|Layout")
	int32 WallBracketStep = 5;
};
