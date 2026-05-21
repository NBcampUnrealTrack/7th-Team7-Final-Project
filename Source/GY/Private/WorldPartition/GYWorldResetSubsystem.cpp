// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldPartition/GYWorldResetSubsystem.h"

#include "WorldPartition/GYActorGuidDataSettings.h"
#include "WorldPartition/LevelPlacedActorData.h"
#include "WorldPartition/WorldPartitionLevelPlacedActor.h"

void UGYWorldResetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const UGYActorGuidDataSettings* ActorGuidDataSettings = GetDefault<UGYActorGuidDataSettings>())
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
	checkf(GetWorld() && GetWorld()->GetAuthGameMode(), TEXT("Has Not Authority"));

	FGuid TargetActorGuid = Actor->GetPersistentGuid();

	if (!ActorGuids.Contains(TargetActorGuid)) return;

	DeactivatedActors.Add(TargetActorGuid);
}

void UGYWorldResetSubsystem::OnActorBeginPlay(IWorldPartitionLevelPlacedActor* Actor)
{
	checkf(GetWorld() && GetWorld()->GetAuthGameMode(), TEXT("Has Not Authority"));

	FGuid TargetActorGuid = Actor->GetPersistentGuid();
	if (DeactivatedActors.Find(TargetActorGuid))
	{
		Actor->Deactivate();
	}
}

void UGYWorldResetSubsystem::ResetWorld()
{
	DeactivatedActors.Empty();
	//TODO 연출
	//TODO 시간관리는 외부에서
}

