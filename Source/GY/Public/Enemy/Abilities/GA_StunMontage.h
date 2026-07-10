#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_StunMontage.generated.h"

UCLASS()
class GY_API UGA_StunMontage : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_StunMontage();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnLoopBegin();

	UFUNCTION()
	void OnEndSection();

private:
	float CachedLoopRate = 1.f;
};
