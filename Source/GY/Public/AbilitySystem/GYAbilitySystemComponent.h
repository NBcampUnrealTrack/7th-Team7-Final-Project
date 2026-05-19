#pragma once

#include "AbilitySystemComponent.h"
#include "GYAbilitySystemComponent.generated.h"

class UGYPeriodicAttributeEffect;

UCLASS()
class GY_API UGYAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	void RescheduleStaminaRegen();
	void RescheduleHitResRegen();
	void ReschedulePoiseRegen();

	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaminaRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> HitResRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> PoiseRegenEffect;

private:
	void OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount);

	void ScheduleEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle, FTimerHandle& DelayHandle, void(UGYAbilitySystemComponent::* StartFunc)());
	void ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle);
	void StopEffect(FActiveGameplayEffectHandle& Handle, FTimerHandle& DelayHandle);

	void StartStaminaRegen();
	void StartHitResRegen();
	void StartPoiseRegen();

	FTimerHandle StaminaRegenDelayHandle;
	FActiveGameplayEffectHandle StaminaRegenGEHandle;
	FTimerHandle HitResRegenDelayHandle;
	FActiveGameplayEffectHandle HitResRegenGEHandle;
	FTimerHandle PoiseRegenDelayHandle;
	FActiveGameplayEffectHandle PoiseRegenGEHandle;
};
