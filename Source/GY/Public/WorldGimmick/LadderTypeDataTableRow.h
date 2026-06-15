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
	TObjectPtr<UStaticMesh> TopCapMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ladder|Layout", meta=(ClampMin=10))
	float RungSpacing = 30.f;
};
