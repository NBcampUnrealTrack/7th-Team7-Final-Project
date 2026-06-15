#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_LadderActivate.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGA_LadderActivate : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_LadderActivate(const FObjectInitializer& ObjectInitializer);

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
