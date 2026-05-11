#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "GYEquipmentSettings.generated.h"

class UDataTable;
class UGameplayEffect;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY Equipment"))
class GY_API UGYEquipmentSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	UPROPERTY(EditAnywhere, Config, Category = "Weapon")
	TSoftObjectPtr<UDataTable> WeaponBaseStatsTable;

	UPROPERTY(EditAnywhere, Config, Category = "Weapon")
	TSubclassOf<UGameplayEffect> BaseATKEffectClass;
};
