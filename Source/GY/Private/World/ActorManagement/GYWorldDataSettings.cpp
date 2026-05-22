#include "World/ActorManagement/GYWorldDataSettings.h"

#include "Logging/GYLogManager.h"



void UGYWorldDataSettings::SetupPath(const FString& ActorGuidTablePath)
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
