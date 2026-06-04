#pragma once

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GYAbilitySystemComponent.generated.h"

class UGYPeriodicAttributeEffect;
class UAnimInstance;
class UAnimMontage;

UCLASS()
class GY_API UGYAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyMontageStart(UAnimMontage* Montage, float ServerTimestamp);

	// InputTag으로 매칭되는 어빌리티 입력 (Lyra 라우팅)
	// 누름/뗌은 SpecHandle 집합에 기록만 하고, 실제 처리는 PostProcessInput의 ProcessAbilityInput에서.
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	void ClearAbilityInput();

	void HandleAbilityInputPressed(const FGameplayTag& InputTag);
	void HandleAbilityInputReleased(const FGameplayTag& InputTag);

	// 클라(UI 등)에서 서버 ASC로 임의 GameplayEvent 전달 (예: 상호작용 중 휴식 어빌리티 종료)
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SendGameplayEvent(FGameplayTag EventTag, FGameplayEventData Payload);

	void RescheduleStaminaRegen();
	void RescheduleStaggerRegen();
	void RescheduleStunRegen();

	void ApplyCombatTag();
	void NotifyAttributeDecreased(const FGameplayAttribute& Attribute);

	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaminaRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaggerRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StunRegenEffect;

	UPROPERTY(EditDefaultsOnly, Category="GAS")
	FGameplayTagContainer CombatAppliedTags;

	UPROPERTY(EditDefaultsOnly, Category="GAS")
	FGameplayTag RegenAppliedTag;

	UPROPERTY(EditDefaultsOnly, Category="GAS", meta=(ClampMin="0.0", Units="s"))
	float CombatAppliedDuration = 5.f;

protected:
	virtual void OnRep_ReplicatedAnimMontage() override;

private:
	TMap<UAnimMontage*, float> MontageStartCache;
	FTimerHandle CatchUpTimerHandle;

	void ApplyMontageCorrection(UAnimInstance* AnimInst, UAnimMontage* Montage, float StartTime);
	void OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount);
	void TryActivateAbilitiesOnSpawn();

	FTimerHandle CombatTagTimer;

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

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
