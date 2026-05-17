// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GYWorldRegionSubsystem.generated.h"

class IWorldPartitionLevelPlacedActor;
/**
 *
 */
UCLASS()
class GY_API UGYWorldRegionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void OnActorDeactivated(IWorldPartitionLevelPlacedActor* Actor);
	void OnActorBeginPlay(IWorldPartitionLevelPlacedActor* Actor);
	void OnRegionReset(FName RegionName);

	//SaveLoad
	FORCEINLINE const TSet<FGuid>& GetDeactivatedActors() const { return DeactivatedActors; }
	FORCEINLINE void SetDeactivatedActors(const TSet<FGuid>& InDeactivatedActors){DeactivatedActors = InDeactivatedActors;};
	FORCEINLINE TMap<FName, FDateTime> GetRegionResetTimes() const { return RegionResetTimes; }
	FORCEINLINE void SetRegionResetTimes(const TMap<FName, FDateTime>& InRegionResetTimes) { RegionResetTimes = InRegionResetTimes; }

private:
	void TryResetRegion(FName RegionName);

	const float RegionResetDelay = 15.0f;

	TMap<FGuid, FName> ActorRegionMap;
	TMap<FName, TArray<FGuid>> ActorsInRegion;
	TSet<FName> Regions;

	TSet<FGuid> DeactivatedActors;
	TMap<FName, FDateTime> RegionResetTimes;
};
