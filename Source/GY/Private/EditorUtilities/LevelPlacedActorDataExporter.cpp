// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelPlacedActorDataExporter.h"

#include "AssetSelection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/GYLogManager.h"
#include "UObject/SavePackage.h"
#include "WorldPartition/ActorDescContainerInstance.h"
#include "WorldPartition/LevelPlacedActorData.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"
#include "WorldPartition/WorldPartitionLevelPlacedActor.h"

ULevelPlacedActorDataExporter::ULevelPlacedActorDataExporter()
{
	SupportedClasses.Add(ULevel::StaticClass());
}

void ULevelPlacedActorDataExporter::ExportMonsterData()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		GY_LOG(Game, JCM, "World is null");
		return;
	}

	UWorldPartition* WP = World->GetWorldPartition();
	if (!WP)
	{
		GY_LOG(Game, JCM, "World Partition is null");
		return;
	}

	TArray<FAssetData> SelectedAssets;

	AssetSelectionUtils::GetSelectedAssets(SelectedAssets);
	for (const FAssetData& AssetData : SelectedAssets)
	{
		UWorld* TargetWorld = Cast<UWorld>(AssetData.GetAsset());
		if (!TargetWorld) continue;

		CollectActorsFromWorld(TargetWorld);
	}

	GY_LOG(Game, JCM, "ULevelPlacedActorDataExporter: %d DataLayer Collected", DataLayerMonsterMap.Num());

	bool bSuccess = true;
	if (UDataTable* SpawnTable = BuildActorRegionTable())
	{
		bSuccess &= SaveDataTable(SpawnTable, OutputPath / TEXT("DT_ActorDataLayerInfo"));
	}
	if (UDataTable* SummaryTable = BuildRegionSummaryTable())
	{
		bSuccess &= SaveDataTable(SummaryTable, OutputPath / TEXT("DT_DataLayerSummary"));
	}

	GY_LOG(Game, JCM, "MonsterDataExporter: %s", bSuccess ? TEXT("완료") : TEXT("일부 저장 실패"));

}

void ULevelPlacedActorDataExporter::CollectActorsFromWorld(UWorld* World)
{
	DataLayerMonsterMap.Empty();

	UWorldPartition* WP = World->GetWorldPartition();

	WP->ForEachActorDescContainerInstanceBreakable([&](UActorDescContainerInstance* Container) -> bool
	{
		for (const auto& [Guid, DescPtr] : Container->GetActorsByGuid())
		{
			if (!DescPtr || !DescPtr->Get()) continue;

			const FWorldPartitionActorDescInstance& DescInstance = *DescPtr->Get();
			const FWorldPartitionActorDesc* ActorDesc = DescInstance.GetActorDesc();

			if (!ActorDesc) continue;

			FSoftObjectPath ClassPath = ActorDesc->GetActorSoftPath();
			UObject* LoadedAsset = ClassPath.TryLoad();

			if (!LoadedAsset)continue;

			UClass* ActorClass ;

			if (UBlueprint* BP = Cast<UBlueprint>(LoadedAsset))
			{
				ActorClass = BP->GeneratedClass;
			}
			else
			{
				ActorClass = LoadedAsset->GetClass();
			}

			;
			if (!ActorClass || !ActorClass->ImplementsInterface(UWorldPartitionLevelPlacedActor::StaticClass()))
				continue;
			FActorRegionTableRow Row;
			Row.ActorGuid = ActorDesc->GetGuid();

			checkf(DescInstance.GetDataLayerInstanceNames().Num() == 1, TEXT("액터가 여러 DataLayer에 중복으로 들어가있음"));

			for (const FName& LayerName : DescInstance.GetDataLayers())
				Row.RegionName = LayerName;

			DataLayerMonsterMap.FindOrAdd(Row.RegionName).Add(Row);
		}
		return true;
	});
}

UDataTable* ULevelPlacedActorDataExporter::BuildActorRegionTable()
{
	UPackage* Pkg = CreatePackage(*(OutputPath / TEXT("DT_ActorRegionData")));
	UDataTable* Table = NewObject<UDataTable>(Pkg, FName("DT_ActorRegionData"), RF_Public | RF_Standalone);
	Table->RowStruct = FActorRegionTableRow::StaticStruct();

	int32 Idx = 0;
	for (const auto& [Layer, Rows] : DataLayerMonsterMap)
	{
		for (const FActorRegionTableRow& Row : Rows){
			Table->AddRow(FName(*FString::Printf(TEXT("Monster_%04d"), Idx++)), Row);
		}
	}

	return Table;
}

UDataTable* ULevelPlacedActorDataExporter::BuildRegionSummaryTable()
{
	UPackage* Pkg = CreatePackage(*(OutputPath / TEXT("DT_DataLayerSummary")));
	UDataTable* Table = NewObject<UDataTable>(Pkg, FName("DT_DataLayerSummary"), RF_Public | RF_Standalone);
	Table->RowStruct = FRegionDataTableRow::StaticStruct();

	for (const auto& [LayerName, Monsters] : DataLayerMonsterMap)
	{
		FRegionDataTableRow Row;
		Row.RegionName = LayerName;
		Table->AddRow(LayerName, Row);
	}
	return Table;
}

bool ULevelPlacedActorDataExporter::SaveDataTable(UDataTable* DataTable, const FString& AssetPath)
{
	if (!DataTable) return false;

	UPackage* Pkg = DataTable->GetPackage();
	Pkg->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(DataTable);

	const FString Filename = FPackageName::LongPackageNameToFilename(
		AssetPath, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	const bool bSaved = UPackage::SavePackage(Pkg, DataTable, *Filename, SaveArgs);
	GY_LOG(Game, JCM, "  %s: %s", bSaved ? TEXT("SaveSuccess") : TEXT("SaveFail"), *Filename);
	return bSaved;
}
