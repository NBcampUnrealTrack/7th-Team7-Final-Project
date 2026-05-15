#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "EnemyTypeTableRow.generated.h"

USTRUCT(BlueprintType)
struct FEnemyTypeTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 로드할 DataAsset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TSoftObjectPtr<UEnemyDataAsset> DataAsset;

	/** 스탯 DataTable Row 키 (DataTable에서 스탯 조회 시 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FName StatRowName;
};
