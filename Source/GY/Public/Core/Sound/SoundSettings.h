#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SoundSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Sound Settings"))
class GY_API USoundSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USoundSettings();

	UPROPERTY(Config, EditAnywhere, Category="Sound")
	TSoftObjectPtr<UDataTable> SoundDataTable;

	static const USoundSettings* Get() { return GetDefault<USoundSettings>(); }
};
