#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RegionLootData.generated.h"

class UDataTable;

UCLASS(BlueprintType, Const, Meta = (DisplayName = "Region Loot Data"))
class GY_API URegionLootData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Region"))
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FText RegionDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TSoftObjectPtr<UTexture2D> RegionIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GY.ItemPoolRow"))
	TObjectPtr<UDataTable> ItemPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GY.GradeDistributionRow"))
	TObjectPtr<UDataTable> GradeDistribution;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GY.LevelDistributionRow"))
	TObjectPtr<UDataTable> LevelDistribution;


};
