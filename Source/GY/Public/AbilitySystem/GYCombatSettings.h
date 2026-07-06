#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GYCombatSettings.generated.h"

class UGameplayEffect;
class UDataTable;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GY Combat"))
class GY_API UGYCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Game"); }

	// 플레이어 타격 GE (HP는 UGYDamageExecution, 경직/무력은 SetByCaller 모디파이어). ApplyHitImpact가 적용.
	UPROPERTY(EditAnywhere, Config, Category = "Combat")
	TSoftClassPtr<UGameplayEffect> HitImpactEffect;

	// 적 어빌리티별 타격 weight(Additive/Multiplicative/Stagger/Stun) 테이블.
	// 행은 GYEditor 모듈이 BP 저장 시 자동 동기화, 값은 여기서 채움. 어빌리티가 부여될 때 로드.
	UPROPERTY(EditAnywhere, Config, Category = "Combat")
	TSoftObjectPtr<UDataTable> EnemyAbilityWeightTable;
};
