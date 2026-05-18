// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldPartition/GYRegionDataSettings.h"

#include "Logging/GYLogManager.h"

void UGYRegionDataSettings::SetupPath(const FString& ActorRegionTablePath, const FString& RegionTablePath)
{
	FSoftObjectPath ActorRegionPath(*ActorRegionTablePath);
	FSoftObjectPath RegionPath(*RegionTablePath);

	bool bIsDirty = false;

	if (ActorRegionTable.ToSoftObjectPath() != ActorRegionPath)
	{
		ActorRegionTable = TSoftObjectPtr<UDataTable>(ActorRegionPath);
		bIsDirty = true;
	}

	if (RegionDataTable.ToSoftObjectPath() != RegionPath)
	{
		RegionDataTable = TSoftObjectPtr<UDataTable>(RegionPath);
		bIsDirty = true;
	}

	if (bIsDirty)
	{
		SaveConfig();
		TryUpdateDefaultConfigFile();

		GY_LOG(Game, JCM, "UGYRegionDataSettings SetupPath Complete");
	}
}
