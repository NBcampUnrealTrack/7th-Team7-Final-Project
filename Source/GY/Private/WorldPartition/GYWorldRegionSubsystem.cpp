// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldPartition/GYWorldRegionSubsystem.h"

#include "WorldPartition/GYRegionDataSettings.h"
#include "WorldPartition/LevelPlacedActorData.h"
#include "WorldPartition/WorldPartitionLevelPlacedActor.h"

void UGYWorldRegionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const UGYRegionDataSettings* RegionDataSettings = GetDefault<UGYRegionDataSettings>())
	{
		if (RegionDataSettings->ActorRegionTable.LoadSynchronous())
		{
			TArray<FActorRegionTableRow*> ActorRegionTableRows;
			RegionDataSettings->ActorRegionTable->GetAllRows("ActorRegionTable Load", ActorRegionTableRows);
			for (FActorRegionTableRow* ActorRegionTableRow : ActorRegionTableRows)
			{
				ActorRegionMap.Add(ActorRegionTableRow->ActorGuid, ActorRegionTableRow->RegionName);
				ActorsInRegion.FindOrAdd(ActorRegionTableRow->RegionName).Add(ActorRegionTableRow->ActorGuid);
			}
		}
		if (RegionDataSettings->RegionDataTable.LoadSynchronous())
		{
			TArray<FRegionDataTableRow*> RegionTableRows;
			RegionDataSettings->RegionDataTable->GetAllRows<FRegionDataTableRow>(
				"RegionDataTable Load", RegionTableRows);
			for (FRegionDataTableRow* RegionTableRow : RegionTableRows)
			{
				Regions.Add(RegionTableRow->RegionName);
			}
		}
	}
}

void UGYWorldRegionSubsystem::OnActorDeactivated(IWorldPartitionLevelPlacedActor* Actor)
{
	checkf(GetWorld() && GetWorld()->GetAuthGameMode(), TEXT("Has Not Authority"));

	if (ActorRegionMap.IsEmpty()) return;

	FGuid TargetActorGuid = Actor->GetPersistentGuid();

	FName* RegionName = ActorRegionMap.Find(TargetActorGuid);

	if (!RegionName ) return;
	DeactivatedActors.Add(TargetActorGuid);

	if (nullptr == RegionResetTimes.Find(*RegionName))
	{
		RegionResetTimes.Add(*RegionName, FDateTime::Now() + FTimespan::FromSeconds(RegionResetDelay));
	}
}

void UGYWorldRegionSubsystem::OnActorBeginPlay(IWorldPartitionLevelPlacedActor* Actor)
{
	checkf(GetWorld() && GetWorld()->GetAuthGameMode(), TEXT("Has Not Authority"));

	if (ActorRegionMap.IsEmpty()) return;

	FGuid TargetActorGuid = Actor->GetPersistentGuid();
	FName* RegionName = ActorRegionMap.Find(TargetActorGuid);
	if (!RegionName) return;


	TryResetRegion(*RegionName);

	if (nullptr == RegionResetTimes.Find(*RegionName)) //지역이 이미 초기화된 경우
	{
		if (DeactivatedActors.Find(TargetActorGuid))
		{
			DeactivatedActors.Remove(TargetActorGuid);
		}
		Actor->Activate();

	}
	else //지역이 초기화 안된경우
	{
		if (DeactivatedActors.Find(TargetActorGuid))
		{
			Actor->Deactivate();
		}
		else
		{
			return;
		}
	}

}

void UGYWorldRegionSubsystem::OnRegionReset(FName RegionName)
{

	if (false == RegionResetTimes.Contains(RegionName)) return;
	if (ActorsInRegion.IsEmpty()) return;

	RegionResetTimes.Remove(RegionName);

	TArray<FGuid>* Guids = ActorsInRegion.Find(RegionName);
	if (!Guids || Guids->IsEmpty()) return;

	for (FGuid ActorGuid : *Guids)
	{
		if (DeactivatedActors.Contains(ActorGuid))
		{
			DeactivatedActors.Remove(ActorGuid);
		}
	}
}

void UGYWorldRegionSubsystem::TryResetRegion(FName RegionName)
{
	if (!RegionResetTimes.Contains(RegionName)) return;

	if (RegionResetTimes[RegionName] < FDateTime::Now())
	{
		OnRegionReset(RegionName);
	}
}
