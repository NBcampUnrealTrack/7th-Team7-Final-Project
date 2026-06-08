// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGimmick/TimeRift/TimeRiftSubsystem.h"

#include "World/ActorManagement/GYWorldDataSettings.h"
#include "World/ActorManagement/RespawnPointTableRow.h"
#include "WorldGimmick/TimeRift/TimeRift.h"


void UTimeRiftSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BuildStaticCacheFromBakeTable();
}

void UTimeRiftSubsystem::BuildStaticCacheFromBakeTable()
{
    StaticDatas.Empty();

    const UGYWorldDataSettings* Settings = GetDefault<UGYWorldDataSettings>();
    if (!Settings) return;

    UDataTable* Table = Settings->RespawnCheckpointTable.LoadSynchronous();
    if (!Table) return;

    TArray<FRespawnPointTableRow*> Rows;
    Table->GetAllRows<FRespawnPointTableRow>(TEXT("TimeRiftSubsystem::BuildStaticCacheFromBakeTable"), Rows);
    for (FRespawnPointTableRow* Row : Rows)
    {
        if (!Row || !Row->ActorGuid.IsValid()) continue;

        FTimeRiftStaticData Entry;
        Entry.Id = Row->ActorGuid;
        Entry.RespawnTransform = Row->RespawnTransform;
        StaticDatas.Add(Entry.Id, Entry);
    }
}

void UTimeRiftSubsystem::RegisterActor(ATimeRift* Rift)
{
    if (!Rift) return;
    const FGuid Id = Rift->GetPersistentGuid();
    if (!Id.IsValid()) return;

    LiveActors.Add(Id, Rift);

    FTimeRiftStaticData& Entry = StaticDatas.FindOrAdd(Id);
    Entry.Id = Id;
    Entry.RespawnTransform = Rift->GetRespawnTransform();
}

void UTimeRiftSubsystem::UnregisterActor(ATimeRift* Rift)
{
    if (!Rift) return;
    LiveActors.Remove(Rift->GetPersistentGuid());
}

ATimeRift* UTimeRiftSubsystem::FindActor(const FGuid& Id) const
{
    if (const TWeakObjectPtr<ATimeRift>* Found = LiveActors.Find(Id))
    {
        return Found->Get();
    }
    return nullptr;
}

bool UTimeRiftSubsystem::TryGetStaticData(const FGuid& Id, FTimeRiftStaticData& Out) const
{
    if (const FTimeRiftStaticData* TimeRiftStaticData = StaticDatas.Find(Id))
    {
        Out = *TimeRiftStaticData;
        return true;
    }
    return false;
}

bool UTimeRiftSubsystem::TryGetRespawnTransform(const FGuid& Id, FTransform& Out) const
{
    if (ATimeRift* Live = FindActor(Id))
    {
        Out = Live->GetRespawnTransform();
        return true;
    }
    if (const FTimeRiftStaticData* TimeRiftStaticData = StaticDatas.Find(Id))
    {
        Out = TimeRiftStaticData->RespawnTransform;
        return true;
    }
    return false;
}

bool UTimeRiftSubsystem::TryGetDynamicData(const FGuid& Id, FTimeRiftDynamicData& Out) const
{
    if (const FTimeRiftDynamicData* TimeRiftDynamicData = DynamicDatas.Find(Id))
    {
        Out = *TimeRiftDynamicData;
        return true;
    }
    return false;
}

void UTimeRiftSubsystem::NotifyDiscovered(const FGuid& Id)
{
    if (!Id.IsValid()) return;

    FTimeRiftDynamicData& TimeRiftDynamicData = DynamicDatas.FindOrAdd(Id);
    if (TimeRiftDynamicData.bDiscovered) return;

    TimeRiftDynamicData.bDiscovered = true;
}

void UTimeRiftSubsystem::NotifyVisited(const FGuid& Id)
{
    if (!Id.IsValid()) return;
    FTimeRiftDynamicData& TimeRiftDynamicData = DynamicDatas.FindOrAdd(Id);
    TimeRiftDynamicData.bDiscovered = true;
}
