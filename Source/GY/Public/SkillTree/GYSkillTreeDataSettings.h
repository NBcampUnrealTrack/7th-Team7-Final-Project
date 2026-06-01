// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GYSkillTreeDataSettings.generated.h"

class USkillTreeDataAsset;
/**
 *
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY SkillTreeData"))
class GY_API UGYSkillTreeDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	UPROPERTY(EditAnywhere, Config, Category = "SkillTree")
	TSoftObjectPtr<USkillTreeDataAsset> SkillTreeDataAsset;
};
