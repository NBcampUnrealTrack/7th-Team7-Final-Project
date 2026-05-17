// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GYRegionDataSettings.generated.h"

/**
 *
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY RegionData"))
class GY_API UGYRegionDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	void SetupPath(const FString& ActorRegionTablePath, const FString& RegionTablePath);

	UPROPERTY(EditAnywhere, Config, Category = "Region")
	TSoftObjectPtr<UDataTable> ActorRegionTable;

	UPROPERTY(EditAnywhere, Config, Category = "Region")
	TSoftObjectPtr<UDataTable> RegionDataTable;
};
