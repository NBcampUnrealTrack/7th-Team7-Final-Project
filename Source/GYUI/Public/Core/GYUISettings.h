#pragma once

#include "Engine/DeveloperSettings.h"
#include "GYUISettings.generated.h"

class UCommonActivatableWidget;
class UGYPrimaryGameLayout;
class UGYLootBoxScreenWidget;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="GY UI"))
class GYUI_API UGYUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Config, Category="UI")
	TSoftClassPtr<UGYPrimaryGameLayout> PrimaryGameLayoutClass;

	UPROPERTY(EditDefaultsOnly, Config, Category="UI")
	TSoftClassPtr<UCommonActivatableWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Config, Category="UI")
	TSoftClassPtr<UGYLootBoxScreenWidget> LootBoxScreenClass;

	UPROPERTY(EditDefaultsOnly, Config, Category="UI")
	TSoftClassPtr<UCommonActivatableWidget> RevivalWidgetClass;
};
