#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_TimeRiftEnchant.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGA_TimeRiftEnchant : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TimeRiftEnchant(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnExitEventReceived(FGameplayEventData Payload);
};
