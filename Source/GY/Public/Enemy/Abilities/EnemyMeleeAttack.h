#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "EnemyMeleeAttack.generated.h"


UCLASS()
class GY_API UEnemyMeleeAttack : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
private:
	UFUNCTION()
	void OnWeaponHit(FGameplayEventData Payload);
public:

private:
	int32 HitCount = 0;

};
