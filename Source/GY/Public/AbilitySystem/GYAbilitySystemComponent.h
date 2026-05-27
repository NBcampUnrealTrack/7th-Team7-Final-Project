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

	// InputTag으로 매칭되는 어빌리티 활성화/입력해제 (Lyra 라우팅)
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	void RescheduleStaminaRegen();
	void RescheduleStaggerRegen();
	void RescheduleStunRegen();

	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaminaRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaggerRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StunRegenEffect;

private:
	void OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount);
	void TryActivateAbilitiesOnSpawn();

	void ScheduleEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle, FTimerHandle& DelayHandle, void(UGYAbilitySystemComponent::* StartFunc)());
	void ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle);
	void StopEffect(FActiveGameplayEffectHandle& Handle, FTimerHandle& DelayHandle);

	void StartStaminaRegen();
	void StartStaggerRegen();
	void StartStunRegen();

	FTimerHandle StaminaRegenDelayHandle;
	FActiveGameplayEffectHandle StaminaRegenGEHandle;
	FTimerHandle StaggerRegenDelayHandle;
	FActiveGameplayEffectHandle StaggerRegenGEHandle;
	FTimerHandle StunRegenDelayHandle;
	FActiveGameplayEffectHandle StunRegenGEHandle;
};
