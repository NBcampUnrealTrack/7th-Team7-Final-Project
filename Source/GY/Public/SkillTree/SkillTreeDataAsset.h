// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillTreeDataAsset.generated.h"

class USkillNodeDataAsset;
/**
 *
 */
UCLASS()
class GY_API USkillTreeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	TArray<TObjectPtr<USkillNodeDataAsset>> RootNodes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	TMap<FName, FVector2D> SkillNodePositions;


	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("SkillTree",GetFName());
	}

};
