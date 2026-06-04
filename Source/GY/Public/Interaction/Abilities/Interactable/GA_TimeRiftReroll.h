#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_TimeRiftReroll.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGA_TimeRiftReroll : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TimeRiftReroll(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnExitEventReceived(FGameplayEventData Payload);
};
