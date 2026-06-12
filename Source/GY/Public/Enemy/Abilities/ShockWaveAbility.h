#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "ShockWaveAbility.generated.h"


UCLASS()
class GY_API UShockWaveAbility : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()
public:
	UShockWaveAbility();

protected:
	UFUNCTION(BlueprintCallable, Category = "Boss|Attack|ShockWave")
	void ExecuteShockWave();
protected:
	/** 충격파 최대 반경 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ShockWave")
	float MaxRadius = 1500.f;

	/** 거리별 강도 감쇠 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ShockWave")
	FRuntimeFloatCurve DamageFalloffCurve;

	/** 충격파 Cue (보스 위치 폴발, 모든 클라 자동 전파) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ShockWave")
	FGameplayTag ShockWaveCueTag;

};
