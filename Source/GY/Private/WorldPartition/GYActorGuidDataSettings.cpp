// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldPartition/GYActorGuidDataSettings.h"

#include "Logging/GYLogManager.h"



void UGYActorGuidDataSettings::SetupPath(const FString& ActorGuidTablePath)
{

	FSoftObjectPath ActorGuidPath(*ActorGuidTablePath);

	bool bIsDirty = false;

	if (ActorGuidTable.ToSoftObjectPath() != ActorGuidPath)
	{
		ActorGuidTable = TSoftObjectPtr<UDataTable>(ActorGuidPath);
		bIsDirty = true;
	}

	if (bIsDirty)
	{
		SaveConfig();
		TryUpdateDefaultConfigFile();

		GY_LOG(Game, JCM, "UGYActorGuidDataSettings SetupPath Complete");
	}
}
