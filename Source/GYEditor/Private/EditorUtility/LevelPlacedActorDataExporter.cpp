// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorUtility/LevelPlacedActorDataExporter.h"

#include "AssetSelection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/GYLogManager.h"

#include "UObject/SavePackage.h"
#include "WorldPartition/ActorDescContainerInstance.h"
#include "WorldPartition/GYRegionDataSettings.h"
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

	const UGYRegionDataSettings* RegionDataSettings = GetDefault<UGYRegionDataSettings>();
	if (!RegionDataSettings)
	{
		GY_ERROR(Game, JCM, "Get RegionDataSettings Fail");
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
		bSuccess &= SaveDataTable(SpawnTable, OutputPath / TEXT("DT_ActorRegionData"));
	}
	if (UDataTable* SummaryTable = BuildRegionSummaryTable())
	{
		bSuccess &= SaveDataTable(SummaryTable, OutputPath / TEXT("DT_RegionSummary"));
	}
	GY_LOG(Game, JCM, "MonsterDataExporter: %s", bSuccess ? TEXT("완료") : TEXT("일부 저장 실패"));

	if (bSuccess)
	{
		if (UGYRegionDataSettings* MutableSettings = GetMutableDefault<UGYRegionDataSettings>())
		{
			MutableSettings->SetupPath(OutputPath/TEXT("DT_ActorRegionData.")+TEXT("DT_ActorRegionData"), OutputPath/TEXT("DT_RegionSummary.")+TEXT("DT_RegionSummary"));
		}
	}

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

			UClass* ActorClass;

			if (UBlueprint* BP = Cast<UBlueprint>(LoadedAsset))
			{
				ActorClass = BP->GeneratedClass;
			}
			else
			{
				ActorClass = LoadedAsset->GetClass();
			};
			if (!ActorClass || !ActorClass->ImplementsInterface(UWorldPartitionLevelPlacedActor::StaticClass()))
				continue;

			AActor* TargetActor = nullptr;

			if (FWorldPartitionActorDescInstance* MutableDescInstance = const_cast<FWorldPartitionActorDescInstance*>(&
				DescInstance))
			{
				TargetActor = MutableDescInstance->GetActor();
			}

			// 2. 액터 변수에 Guid 주역 및 에디터 수정 마킹 (Dirty)
			if (TargetActor)
			{
				// 에디터의 Undo 시스템 등록 및 에셋 변경 마킹 (컨트롤+S로 저장되도록 보장)
				TargetActor->Modify();

				// 인터페이스 형태로 캐스팅하여 Guid 주입 함수 호출
				if (IWorldPartitionLevelPlacedActor* Interface = Cast<IWorldPartitionLevelPlacedActor>(TargetActor))
				{
					// 가공할 고유 Guid (ActorDesc의 Guid를 그대로 사용)
					FGuid ActorGuid = ActorDesc->GetGuid();
					Interface->SetPersistentGuid(ActorGuid);

					GY_LOG(Game, JCM, "Baked Guid [%s] to Actor [%s]", *ActorGuid.ToString(), *TargetActor->GetName());
				}
			}
			else
			{
				GY_ERROR(Game, JCM, "Failed to load Actor Instance for Guid: %s", *ActorDesc->GetGuid().ToString());
				continue;
			}


			FActorRegionTableRow Row;
			Row.ActorGuid = ActorDesc->GetGuid();

			TArray<FName> DataLayers = DescInstance.GetDataLayers();

			if (DataLayers.IsEmpty()) continue;

			if (DataLayers.Num()>1)
			{
				GY_LOG(Game, JCM, "액터가 여러 DataLayer에 중복으로 들어가있음: %s", *ActorDesc->GetGuid().ToString());
			}
			if (DataLayers.Num()>0)
			{
				Row.RegionName = DataLayers[0];
			}

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
		for (const FActorRegionTableRow& Row : Rows)
		{
			Table->AddRow(FName(*FString::Printf(TEXT("Monster_%04d"), Idx++)), Row);
		}
	}

	return Table;
}

UDataTable* ULevelPlacedActorDataExporter::BuildRegionSummaryTable()
{
	UPackage* Pkg = CreatePackage(*(OutputPath / TEXT("DT_RegionSummary")));
	UDataTable* Table = NewObject<UDataTable>(Pkg, FName("DT_RegionSummary"), RF_Public | RF_Standalone);
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
