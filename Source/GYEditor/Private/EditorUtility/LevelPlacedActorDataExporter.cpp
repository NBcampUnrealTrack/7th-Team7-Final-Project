// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorUtility/LevelPlacedActorDataExporter.h"

#include "AssetSelection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/GYLogManager.h"

#include "UObject/SavePackage.h"
#include "WorldPartition/ActorDescContainerInstance.h"
#include "WorldPartition/GYActorGuidDataSettings.h"
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

	const UGYActorGuidDataSettings* ActorGuidDataSettings = GetDefault<UGYActorGuidDataSettings>();
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

	bool bSuccess = true;
	if (UDataTable* SpawnTable = BuildActorRegionTable())
	{
		bSuccess = SaveDataTable(SpawnTable, OutputPath / TEXT("DT_ActorGuidData"));
	}

	GY_LOG(Game, JCM, "MonsterDataExporter: %s", bSuccess ? TEXT("완료") : TEXT("일부 저장 실패"));

	if (bSuccess)
	{
		if (UGYActorGuidDataSettings* MutableSettings = GetMutableDefault<UGYActorGuidDataSettings>())
		{
			MutableSettings->SetupPath(OutputPath/TEXT("DT_ActorGuidData.")+TEXT("DT_ActorGuidData"));
		}
	}

}

void ULevelPlacedActorDataExporter::CollectActorsFromWorld(UWorld* World)
{
	ActorGuidSet.Empty();

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

			if (TargetActor)
			{
				TargetActor->Modify();

				if (IWorldPartitionLevelPlacedActor* Interface = Cast<IWorldPartitionLevelPlacedActor>(TargetActor))
				{
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


			ActorGuidSet.Add(ActorDesc->GetGuid());
		}
		return true;
	});
}

UDataTable* ULevelPlacedActorDataExporter::BuildActorRegionTable()
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
