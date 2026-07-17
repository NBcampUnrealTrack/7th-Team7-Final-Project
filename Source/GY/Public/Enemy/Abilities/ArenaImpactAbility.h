#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "ArenaImpactAbility.generated.h"

class AAreaImpactProjectile;

UCLASS()
class GY_API UArenaImpactAbility : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION()
	void OnProjectileHit(FGameplayEventData Payload);

	void DoImpact();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ArenaImpact")
	TSubclassOf<AAreaImpactProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ArenaImpact")
	FName AnchorActorTag = TEXT("TentacleAnchor");

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ArenaImpact")
	FName ArenaCenterTag = TEXT("ArenaCenter");

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ArenaImpact", meta = (ClampMin = "0.0"))
	float ImpactDelay = 0.f;

private:
	FTimerHandle ImpactTimerHandle;
	TWeakObjectPtr<AAreaImpactProjectile> SpawnedProjectile;
};
