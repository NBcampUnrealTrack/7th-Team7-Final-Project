#pragma once

#include "Engine/DeveloperSettings.h"
#include "GYUISettings.generated.h"

class UCommonActivatableWidget;
class UGYPrimaryGameLayout;
class UGYLootBoxScreenWidget;
class UGYEndingCreditsWidget;
class UGYInteractionWaitingWidget;
class UGYWorldResetWidget;

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

	UPROPERTY(EditDefaultsOnly, Config, Category="UI|WorldReset")
	TSoftClassPtr<UGYWorldResetWidget> WorldResetWidgetClass;

	UPROPERTY(EditDefaultsOnly, Config, Category="UI|Ending")
	TSoftClassPtr<UGYEndingCreditsWidget> EndingCreditsWidgetClass;

	UPROPERTY(EditDefaultsOnly, Config, Category="UI|Ending")
	TSoftClassPtr<UGYInteractionWaitingWidget> InteractionWaitingWidgetClass;

	UPROPERTY(EditDefaultsOnly, Config, Category="UI|Ending", meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath MainMenuMap;
};
