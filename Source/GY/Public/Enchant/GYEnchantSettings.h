#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GYEnchantSettings.generated.h"

class UDataTable;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY Enchant"))
class GY_API UGYEnchantSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	UPROPERTY(EditAnywhere, Config, Category = "Enchant")
	TSoftObjectPtr<UDataTable> EnchantCostTable;

	UPROPERTY(EditAnywhere, Config, Category = "Enchant")
	FName DefaultCostRowName = FName("Default");

	UPROPERTY(EditAnywhere, Config, Category = "Enchant")
	TSoftObjectPtr<UDataTable> PenaltyOptionTable;

	// MagnitudeTag → 적용 GE 매핑. 행 키 = MagnitudeTag 전체 이름. 매핑 없는 매그니튜드는 장착 시 미적용
	UPROPERTY(EditAnywhere, Config, Category = "Enchant")
	TSoftObjectPtr<UDataTable> MagnitudeEffectTable;
};
