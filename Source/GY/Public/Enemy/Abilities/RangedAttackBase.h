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

private:
	UFUNCTION()
	void OnLaunchEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnProjectileHit(FGameplayEventData Payload);

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

	// true면 각 탄 데미지를 1/ProjectileCount로 분산
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Projectile|Damage")
	bool bDistributeDamage = true;
};

