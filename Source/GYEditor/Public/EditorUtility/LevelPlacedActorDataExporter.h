// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/AssetActionUtility.h"
#include "WorldPartition/LevelPlacedActorData.h"
#include "LevelPlacedActorDataExporter.generated.h"

/**
 *
 */
UCLASS(BlueprintType, Blueprintable, meta = (ShowWorldContext))
class GYEDITOR_API ULevelPlacedActorDataExporter : public UAssetActionUtility
{
	GENERATED_BODY()

public:
	ULevelPlacedActorDataExporter();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActorGuidData")
	FString OutputPath = TEXT("/Game/GY/Data/Tables/ActorGuidData");

private:
	UFUNCTION(CallInEditor, Category = "ActorGuidData")
	void ExportMonsterData();

private:
#if WITH_EDITOR
	TSet<FGuid> ActorGuidSet;

	void CollectActorsFromWorld(UWorld* World);
	UDataTable* BuildActorRegionTable();
	UDataTable* BuildRegionSummaryTable();
	bool SaveDataTable(UDataTable* DataTable, const FString& AssetPath);
#endif
};
