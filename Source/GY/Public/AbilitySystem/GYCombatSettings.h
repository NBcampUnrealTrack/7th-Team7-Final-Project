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

	// 플레이어 공격 데미지 GE (UGYDamageExecution 사용). ApplyHitImpact가 적용.
	UPROPERTY(EditAnywhere, Config, Category = "Combat")
	TSoftClassPtr<UGameplayEffect> DamageEffect;
};
