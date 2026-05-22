// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GYWorldDataSettings.generated.h"

/**
 *
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY WorldData"))
class GY_API UGYWorldDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	void SetupPath(const FString& ActorGuidTablePath);

	UPROPERTY(EditAnywhere, Config, Category = "ActorGuid")
	TSoftObjectPtr<UDataTable> ActorGuidTable;

	UPROPERTY(EditAnywhere, Config, Category = "ActorGuid")
	TSoftObjectPtr<UDataLayerAsset> DataLayerAsset;

	UPROPERTY(EditAnywhere, Config, Category = "WorldTime")
	float StartOfDayHour = 6.f;
	UPROPERTY(EditAnywhere, Config, Category = "WorldTime")
	float EndOfDayHour = 24.f;
	UPROPERTY(EditAnywhere, Config, Category = "WorldTime")
	float RealMinutesPerGameDay = 20.f;


};
