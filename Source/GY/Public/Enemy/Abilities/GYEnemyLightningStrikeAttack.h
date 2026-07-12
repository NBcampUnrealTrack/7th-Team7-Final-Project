#pragma once

#include "CoreMinimal.h"
#include "Enemy/Abilities/GYEnemyComboAttack.h"
#include "GYEnemyLightningStrikeAttack.generated.h"

class ATargetMarkerProjectile;
class AAreaImpactProjectile;

UCLASS()
class GY_API UGYEnemyLightningStrikeAttack : public UGYEnemyComboAttack
{
	GENERATED_BODY()

protected:
	static constexpr int32 StartStep  = 0;
	static constexpr int32 LoopStep   = 1;
	static constexpr int32 StrikeStep = 2;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void OnLaunchProjectile(FGameplayEventData Payload) override;
	virtual void OnComboMontageEnded() override;

	UFUNCTION()
	void OnMarkerArrived(FGameplayEventData Payload);

	void SpawnMarker();
	void SpawnStrikeProjectile();

	// 마커가 이동할 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "Lightning")
	float MaxMarkerDistance = 500.f;

	// 타겟 위치로 날아가는 마커 Projectile 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Lightning")
	TSubclassOf<ATargetMarkerProjectile> MarkerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lightning")
	float MarkerSpeed = 800.f;

	// 마커 스폰 시 바닥에서 띄울 높이
	UPROPERTY(EditDefaultsOnly, Category = "Lightning")
	float MarkerGroundOffset = 30.f;

	// 도착 지점에 스폰되는 낙뢰 Projectile 클래스. 스폰 즉시 범위 판정된다
	UPROPERTY(EditDefaultsOnly, Category = "Lightning")
	TSubclassOf<AAreaImpactProjectile> StrikeClass;

	// 낙하 지점 예고 액터(데칼 등). None이면 표시 안 함
	UPROPERTY(EditDefaultsOnly, Category = "Lightning")
	TSubclassOf<AActor> WarningActorClass;

private:
	FVector StrikeLocation = FVector::ZeroVector;
	bool bMarkerArrived = false;
};
