#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GYCombatSettings.generated.h"

class UGameplayEffect;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY Combat"))
class GY_API UGYCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	// 플레이어 타격 GE (HP는 UGYDamageExecution, 경직/무력은 SetByCaller 모디파이어). ApplyHitImpact가 적용.
	UPROPERTY(EditAnywhere, Config, Category = "Combat")
	TSoftClassPtr<UGameplayEffect> HitImpactEffect;
};
