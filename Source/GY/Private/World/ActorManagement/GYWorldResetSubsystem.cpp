#include "World/ActorManagement/GYWorldResetSubsystem.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "World/ActorManagement/LevelPlacedActorData.h"
#include "World/ActorManagement/WorldPartitionLevelPlacedActor.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"

void UGYWorldResetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (const UGYWorldDataSettings* ActorGuidDataSettings = GetDefault<UGYWorldDataSettings>())
	{
		if (ActorGuidDataSettings->ActorGuidTable.LoadSynchronous())
		{
			TArray<FActorGuidTableRow*> ActorGuidTableRows;
			ActorGuidDataSettings->ActorGuidTable->GetAllRows("ActorGuidTable Load", ActorGuidTableRows);
			for (FActorGuidTableRow* ActorGuidTableRow : ActorGuidTableRows)
			{
				ActorGuids.Add(ActorGuidTableRow->ActorGuid);
			}
		}
	}
}

void UGYWorldResetSubsystem::OnActorDeactivated(IWorldPartitionLevelPlacedActor* Actor)
{
	if (!GetWorld() || !GetWorld()->GetAuthGameMode())
	{
		return;
	}
	FGuid TargetActorGuid = Actor->GetPersistentGuid();

	if (!ActorGuids.Contains(TargetActorGuid)) return;

	DeactivatedActors.Add(TargetActorGuid);
}

bool UGYWorldResetSubsystem::OnActorBeginPlay(IWorldPartitionLevelPlacedActor* Actor)
{
	if (!GetWorld() || !GetWorld()->GetAuthGameMode())
	{
		return false;
	}
	FGuid TargetActorGuid = Actor->GetPersistentGuid();
	if (DeactivatedActors.Find(TargetActorGuid))
	{
		Actor->Deactivate();
		return false;
	}
	else
	{
		Actor->Activate();
		return true;
	}
}

void UGYWorldResetSubsystem::ResetWorld()
{

	if (!GetWorld() || !GetWorld()->GetAuthGameMode())
	{
		return;
	}
	DeactivatedActors.Empty();

	UDataLayerManager* DataLayerManager = UDataLayerManager::GetDataLayerManager(GetWorld());
	checkf(DataLayerManager, TEXT("DataLayerManager is NULL"));

	if (const UGYWorldDataSettings* ActorGuidDataSettings = GetDefault<UGYWorldDataSettings>())
	{
		ActorGuidDataSettings->DataLayerAsset.LoadSynchronous();

		DataLayerManager->SetDataLayerRuntimeState(
			ActorGuidDataSettings->DataLayerAsset.Get(),
			EDataLayerRuntimeState::Unloaded);
	}

	FTimerHandle ReloadTimer;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, [DataLayerManager, this]()
	{
		if (const UGYWorldDataSettings* ActorGuidDataSettings = GetDefault<UGYWorldDataSettings>())
		{
			DataLayerManager->SetDataLayerRuntimeState(
				ActorGuidDataSettings->DataLayerAsset.Get(),
				EDataLayerRuntimeState::Activated);
		}
	}, 0.5f, false);
	//TODO 연출
}

