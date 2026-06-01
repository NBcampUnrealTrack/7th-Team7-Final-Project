// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GYWorldResetSubsystem.generated.h"

class IWorldPartitionLevelPlacedActor;
/**
 *
 */
UCLASS()
class GY_API UGYWorldResetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void OnActorDeactivated(IWorldPartitionLevelPlacedActor* Actor);
	//true일 경우 살아있는 액터 false일 경우 죽은 액터(deactivate처리해야함)
	bool OnActorBeginPlay(IWorldPartitionLevelPlacedActor* Actor);
	void ResetWorld();



	//SaveLoad
	FORCEINLINE const TSet<FGuid>& GetDeactivatedActors() const { return DeactivatedActors; }
	FORCEINLINE void SetDeactivatedActors(const TSet<FGuid>& InDeactivatedActors){DeactivatedActors = InDeactivatedActors;};
	FORCEINLINE FDateTime GetWorldResetTime() const { return WorldResetRemain; }
	FORCEINLINE void SetWorldResetTime(const FDateTime& InWorldResetTime) { WorldResetRemain = InWorldResetTime; }

private:

	TSet<FGuid> ActorGuids;
	TSet<FGuid> DeactivatedActors;
	FDateTime WorldResetRemain;
};
