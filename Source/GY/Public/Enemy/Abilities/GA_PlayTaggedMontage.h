#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_PlayTaggedMontage.generated.h"

UCLASS()
class GY_API UGA_PlayTaggedMontage : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_PlayTaggedMontage();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageInterrupted();
};
