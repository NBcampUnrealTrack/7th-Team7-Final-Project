// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TimeRiftData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimeRiftSubsystem.generated.h"

struct FGameplayTag;
class ATimeRift;
/**
 *
 */
UCLASS()
class GY_API UTimeRiftSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void RegisterActor(ATimeRift* Rift);
	void UnregisterActor(ATimeRift* Rift);

	ATimeRift* FindActor(const FGuid& Id) const;
	bool TryGetStaticData(const FGuid& Id, FTimeRiftStaticData& Out) const;
	bool TryGetRespawnTransform(const FGuid& Id, FTransform& Out) const;
	bool TryGetDynamicData(const FGuid& Id, FTimeRiftDynamicData& Out) const;

	void NotifyDiscovered(const FGuid& Id);
	void NotifyVisited(const FGuid& Id);

private:
	void BuildStaticCacheFromBakeTable();

	UPROPERTY()
	TMap<FGuid, FTimeRiftStaticData> StaticDatas;

	UPROPERTY()
	TMap<FGuid, TWeakObjectPtr<ATimeRift>> LiveActors;

	//TODO 저장해야함
	UPROPERTY()
	TMap<FGuid, FTimeRiftDynamicData> DynamicDatas;
};
