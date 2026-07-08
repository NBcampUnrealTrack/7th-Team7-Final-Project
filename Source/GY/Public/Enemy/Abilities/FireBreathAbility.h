#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "FireBreathAbility.generated.h"

UCLASS()
class GY_API UFireBreathAbility : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION() void OnBreathTick(FGameplayEventData Payload);
	UFUNCTION() void OnBreathStart(FGameplayEventData Payload);
	UFUNCTION() void OnBreathEnd(FGameplayEventData Payload);

	void ExecuteConeHit();

public:
	UPROPERTY(EditDefaultsOnly, Category="FireBreath")
	FName MouthSocket = TEXT("mouth");

	UPROPERTY(EditDefaultsOnly, Category="FireBreath", meta=(ClampMin="0"))
	float ConeLength = 500.f;

	UPROPERTY(EditDefaultsOnly, Category="FireBreath", meta=(ClampMin="0", ClampMax="180"))
	float ConeAngleDeg = 45.f;

};
