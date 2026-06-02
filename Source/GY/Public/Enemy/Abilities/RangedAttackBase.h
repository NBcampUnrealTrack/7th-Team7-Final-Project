#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "RangedAttackBase.generated.h"

class AProjectileBase;

UCLASS()
class GY_API URangedAttackBase : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()

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

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (Categories = "GameplayCue"))
	FGameplayTag HitCueTag;
};

