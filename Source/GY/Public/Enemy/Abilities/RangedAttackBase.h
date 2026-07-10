#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "RangedAttackBase.generated.h"

class AProjectileBase;

UCLASS()
class GY_API URangedAttackBase : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()
public:
	virtual bool CanAttackDistance(AActor* Owner, AActor* Target) override;
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	virtual void OnLaunchEvent(FGameplayEventData Payload);

	UFUNCTION()
	virtual void OnProjectileHit(FGameplayEventData Payload);

	/** ProjectileResolveExtraTime > 0이면 몽타주 종료 시 바로 EndAbility하지 않고 투사체 Hit 이벤트를 계속 대기 */
	virtual void OnMontageFinished() override;

	UFUNCTION()
	void OnProjectileResolveTimeout();

	void SpawnProjectile();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Projectile")
	float ProjectileSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Projectile")
	FName LaunchSocket = TEXT("weapon_tip");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Projectile|Spread", meta = (ClampMin = "1"))
	int32 ProjectileCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Projectile|Spread", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SpreadAngle = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Projectile|Spread", meta=(ClampMin="0"))
	float TargetLocationSpread = 0.f;

	// true면 각 탄 데미지를 1/ProjectileCount로 분산
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Projectile|Damage")
	bool bDistributeDamage = true;

	/**
	 * 몽타주 종료 후 어빌리티를 추가로 살려둘 시간(초).
	 * 폭탄처럼 지연 폭발하는 투사체는 Hit 이벤트가 몽타주 종료 뒤에 도착하므로
	 * (비행 최대시간 + FuseTime + 여유)보다 길게 설정해야 데미지가 들어간다. 0이면 기존처럼 즉시 종료.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Projectile|Damage", meta = (ClampMin = "0.0"))
	float ProjectileResolveExtraTime = 0.f;
};

