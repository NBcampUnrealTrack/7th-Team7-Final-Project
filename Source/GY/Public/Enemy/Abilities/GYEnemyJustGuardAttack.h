#pragma once

#include "CoreMinimal.h"
#include "GYEnemyComboAttack.h"
#include "GYEnemyJustGuardAttack.generated.h"

UCLASS()
class GY_API UGYEnemyJustGuardAttack : public UGYEnemyComboAttack
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnJustGuardSuccess(FGameplayEventData Payload);
};
