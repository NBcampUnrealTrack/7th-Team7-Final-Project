// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorUtility/LevelPlacedActorDataExporter.h"

#include "AssetSelection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/GYLogManager.h"

#include "UObject/SavePackage.h"
#include "WorldPartition/ActorDescContainerInstance.h"
#include "World/ActorManagement//GYWorldDataSettings.h"
#include "World/ActorManagement//LevelPlacedActorData.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"
#include "World/ActorManagement//WorldPartitionLevelPlacedActor.h"
#include "World/ActorManagement/RespawnPoint.h"
#include "World/ActorManagement/RespawnPointTableRow.h"

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

	const UGYWorldDataSettings* ActorGuidDataSettings = GetDefault<UGYWorldDataSettings>();
	if (!ActorGuidDataSettings)
	{
		GY_ERROR(Game, JCM, "Get ActorGuidDataSettings Fail");
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

	GY_LOG(Game, JCM, "ULevelPlacedActorDataExporter: %d Actor Collected", ActorGuidSet.Num());

	UGYWorldDataSettings* MutableSettings = GetMutableDefault<UGYWorldDataSettings>();

	if (UDataTable* ActorTable = BuildActorGuidTable())
	{
		const FString AssetName = TEXT("DT_ActorGuidData");
		const FString AssetPath = OutputPath / AssetName;
		if (SaveDataTable(ActorTable, AssetPath) && MutableSettings)
		{
			MutableSettings->SetupPath(AssetPath + TEXT(".") + AssetName);
		}
	}

	if (UDataTable* CheckpointTable = BuildRespawnPointTable())
	{
		const FString AssetName = TEXT("DT_RespawnCheckpoint");
		const FString AssetPath = OutputPath / AssetName;
		if (SaveDataTable(CheckpointTable, AssetPath) && MutableSettings)
		{
			MutableSettings->RespawnCheckpointTable = TSoftObjectPtr<UDataTable>(
				FSoftObjectPath(AssetPath + TEXT(".") + AssetName));
			GY_LOG(Game, JCM, "RespawnCheckpointTable path saved");
		}
	}

	MutableSettings->SaveConfig();
	MutableSettings->TryUpdateDefaultConfigFile();

}

void ULevelPlacedActorDataExporter::CollectActorsFromWorld(UWorld* World)
{
	ActorGuidSet.Empty();
	RespawnCheckpointRows.Empty();

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
			if (!ActorClass) continue;

			const bool bIsLevelPlaced = ActorClass->ImplementsInterface(
				UWorldPartitionLevelPlacedActor::StaticClass());
			const bool bIsCheckpoint = ActorClass->ImplementsInterface(
				URespawnPoint::StaticClass());

			if (!bIsLevelPlaced && !bIsCheckpoint) continue;

			AActor* TargetActor = nullptr;

			if (FWorldPartitionActorDescInstance* MutableDescInstance = const_cast<FWorldPartitionActorDescInstance*>(&
				DescInstance))
			{
				TargetActor = MutableDescInstance->GetActor();
			}
			if (!TargetActor)
			{
				GY_ERROR(Game, JCM, "Failed to load Actor Instance for Guid: %s", *ActorDesc->GetGuid().ToString());
				continue;
			}
			TargetActor->Modify();
			const FGuid ActorGuid = ActorDesc->GetGuid();
			if (bIsLevelPlaced)
			{
				if (IWorldPartitionLevelPlacedActor* Interface = Cast<IWorldPartitionLevelPlacedActor>(TargetActor))
				{
					Interface->SetPersistentGuid(ActorGuid);
					ActorGuidSet.Add(ActorGuid);

					GY_LOG(Game, JCM, "Baked Guid [%s] to Actor [%s]", *ActorGuid.ToString(), *TargetActor->GetName());
				}
			}

			if (bIsCheckpoint)
			{
				if (IRespawnPoint* RespawnPoint = Cast<IRespawnPoint>(TargetActor))
				{
					RespawnPoint->SetPersistentGuid(ActorGuid);

					FRespawnPointTableRow Row;
					Row.ActorGuid = ActorGuid;
					Row.RespawnTransform = RespawnPoint->GetRespawnTransform();

					RespawnCheckpointRows.Add(ActorGuid, Row);

					GY_LOG(Game, JCM, "Baked Checkpoint [%s] to [%s]", *ActorGuid.ToString(), *TargetActor->GetName());
				}
			}

		}
		return true;
	});
}

UDataTable* ULevelPlacedActorDataExporter::BuildActorGuidTable()
{
	UPackage* Pkg = CreatePackage(*(OutputPath / TEXT("DT_ActorGuidData")));
	UDataTable* Table = NewObject<UDataTable>(Pkg, FName("DT_ActorGuidData"), RF_Public | RF_Standalone);
	Table->RowStruct = FActorGuidTableRow::StaticStruct();

	int32 Idx = 0;
	for (const FGuid& TargetActorGuid : ActorGuidSet)
	{
		FActorGuidTableRow Row;
		Row.ActorGuid = TargetActorGuid;
		Table->AddRow(FName(*FString::Printf(TEXT("Monster_%04d"), Idx++)), Row);
	}

	return Table;
}

UDataTable* ULevelPlacedActorDataExporter::BuildRespawnPointTable()
{
	if (RespawnCheckpointRows.IsEmpty()) return nullptr;

	const FString AssetName = TEXT("DT_RespawnCheckpoint");
	UPackage* Pkg = CreatePackage(*(OutputPath / AssetName));
	UDataTable* Table = NewObject<UDataTable>(Pkg, FName(AssetName),
		RF_Public | RF_Standalone);
	Table->RowStruct = FRespawnPointTableRow::StaticStruct();

	int32 Idx = 0;
	for (const auto& Pair : RespawnCheckpointRows)
	{
		Table->AddRow(FName(*FString::Printf(TEXT("Checkpoint_%04d"), Idx++)),
			Pair.Value);
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
