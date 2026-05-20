// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GYActorGuidDataSettings.generated.h"

/**
 *
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY ActorGuidData"))
class GY_API UGYActorGuidDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	void SetupPath(const FString& ActorGuidTablePath);

	UPROPERTY(EditAnywhere, Config, Category = "ActorGuid")
	TSoftObjectPtr<UDataTable> ActorGuidTable;

};
