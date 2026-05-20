#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GYDisassembleSettings.generated.h"

class UDataTable;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY Disassemble"))
class GY_API UGYDisassembleSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	UPROPERTY(EditAnywhere, Config, Category = "Disassemble")
	TSoftObjectPtr<UDataTable> RewardTable;
};
