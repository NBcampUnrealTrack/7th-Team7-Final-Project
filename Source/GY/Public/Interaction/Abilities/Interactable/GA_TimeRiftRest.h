#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_TimeRiftRest.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGA_TimeRiftRest : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TimeRiftRest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnExitEventReceived(FGameplayEventData Payload);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Rest")
	TSubclassOf<UGameplayEffect> RecoveryEffect;
};
