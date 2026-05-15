#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "QuestSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Quest Settings"))
class GY_API UQuestSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UQuestSettings();

	UPROPERTY(Config, EditAnywhere, Category="Quest")
	TSoftObjectPtr<UDataTable> QuestDataTable;

	static const UQuestSettings* Get() { return GetDefault<UQuestSettings>(); }
};