// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/AssetActionUtility.h"
#include "LevelPlacedActorDataExporter.generated.h"

struct FActorRegionTableRow;
/**
 *
 */
UCLASS(BlueprintType, Blueprintable, meta = (ShowWorldContext))
class GY_API ULevelPlacedActorDataExporter : public UAssetActionUtility
{
	GENERATED_BODY()

public:
	ULevelPlacedActorDataExporter();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RegionData")
	FString OutputPath = TEXT("/Game/Data");

private:
	UFUNCTION(CallInEditor, Category = "RegionData")
	void ExportMonsterData();

private:
#if WITH_EDITOR
	TMap<FName, TArray<FActorRegionTableRow>> DataLayerMonsterMap;

	void CollectActorsFromWorld(UWorld* World);
	UDataTable* BuildActorRegionTable();
	UDataTable* BuildRegionSummaryTable();
	bool SaveDataTable(UDataTable* DataTable, const FString& AssetPath);
#endif
};
