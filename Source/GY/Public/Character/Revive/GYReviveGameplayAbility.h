#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GYReviveGameplayAbility.generated.h"

class UReviveProgressComponent;
class AGYCharacter;
class UAnimMontage;

UCLASS()
class GY_API UGYReviveGameplayAbility : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGYReviveGameplayAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	TObjectPtr<UAnimMontage> ReviveLoopMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	TObjectPtr<UAnimMontage> ReviveEndMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	float ReviveRange = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	float MovementCancelThreshold = 10.f;

private:
	void BeginCancel();
	void TickReviveCheck();
	void FinishEnd();

	TWeakObjectPtr<UReviveProgressComponent> ActiveProgress;
	TWeakObjectPtr<AGYCharacter> DownedTarget;
	FTimerHandle CheckTimerHandle;
	bool bCancelInitiated = false;
};
