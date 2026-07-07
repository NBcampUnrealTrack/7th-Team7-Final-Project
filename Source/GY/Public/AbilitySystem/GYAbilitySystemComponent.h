#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/GYAbilityGrantLedger.h"
#include "GYAbilitySystemComponent.generated.h"

class UGYPeriodicAttributeEffect;
class UGYRegenDelayEffect;
class UAnimInstance;
class UAnimMontage;

// 경직/무력화 누적 통이 가득 차면(Current >= Max) 발동되는 행동불능(CC) 설정.
// 발동 시 DisableEffect(지속형 GE)를 적용해 StateTag를 부여하고, 통을 0으로 리셋한다.
// StateTag가 이미 있으면 재발동하지 않는다(latch).
USTRUCT(BlueprintType)
struct GY_API FGYDisableThreshold
{
	GENERATED_BODY()

	// 누적 통 (예: CurrentStun)
	UPROPERTY(EditDefaultsOnly)
	FGameplayAttribute CurrentAttribute;

	// 통의 최대치 (예: MaxStun)
	UPROPERTY(EditDefaultsOnly)
	FGameplayAttribute MaxAttribute;

	// 발동 중임을 나타내는 상태 태그 (예: State.Hit.Stun). 재발동 방지 래치로도 사용.
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag StateTag;

	// 발동 시 자신에게 적용할 지속형 GE. GrantedTags로 StateTag를 부여하고 Duration이 곧 CC 지속시간.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DisableEffect;

	// 발동 시 취소할 진행 중 어빌리티 태그 (예: Ability.Attack)
	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer CancelAbilityTags;

	// 0이면 GE 기본 Duration 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Duration = 0.f;
};

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

	UFUNCTION(Server, Reliable)
	void Server_AdvanceCombo(int32 NewComboIndex);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SendGameplayEvent(FGameplayTag EventTag, FGameplayEventData Payload);

	void ApplyCombatTag();
	void RemoveCombatTag();
	void NotifyAttributeChanged(const FGameplayAttribute& Attribute);

	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaminaRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StaggerRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGYPeriodicAttributeEffect> StunRegenEffect;
	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TSubclassOf<UGameplayEffect> CombatStateEffect;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Regen")
	TSubclassOf<UGYRegenDelayEffect> RegenDelayEffect;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Regen")
	float StaminaRegenDelayDuration = 2.f;
	UPROPERTY(EditDefaultsOnly, Category="GAS|Regen")
	float StaggerRegenDelayDuration = 2.f;
	UPROPERTY(EditDefaultsOnly, Category="GAS|Regen")
	float StunRegenDelayDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category="GAS")
	TArray<FGYDisableThreshold> DisableThresholds;

	// 누적 통이 변할 때 GYVitalAttributeSet이 호출. 가득 차면 해당 CC를 발동한다. [SERVER]
	void HandleVitalAccumulation(const FGameplayAttribute& ChangedAttribute, float CurrentValue);

	UGameplayAbility* GetActiveAbilityByTag(const FGameplayTag& AbilityTag) const;

	bool HasGrantSource(FGameplayTag Source) const { return GrantLedger.HasSource(Source); }

	void Grant_AddLooseTag(FGameplayTag Source, FGameplayTag Tag, int32 Count = 1,
		EGameplayTagReplicationState TagRepState = EGameplayTagReplicationState::None)
	{
		GrantLedger.AddLooseTag(this, Source, Tag, Count, TagRepState);
	}

	FActiveGameplayEffectHandle Grant_ApplyEffectSpec(FGameplayTag Source, const FGameplayEffectSpec& Spec)
	{
		return GrantLedger.ApplyEffectSpec(this, Source, Spec);
	}

	void Grant_AdoptHandles(FGameplayTag Source, const FAbilitySetGrantedHandles& Handles)
	{
		GrantLedger.AdoptHandles(Source, Handles);
	}

	void RevokeGrantSource(FGameplayTag Source) { GrantLedger.RevokeSource(this, Source); }

protected:
	virtual void OnRep_ReplicatedAnimMontage() override;

private:
	UPROPERTY()
	FGYAbilityGrantLedger GrantLedger;

	TMap<UAnimMontage*, float> MontageStartCache;
	FTimerHandle CatchUpTimerHandle;

	void ApplyMontageCorrection(UAnimInstance* AnimInst, UAnimMontage* Montage, float StartTime);
	void TryActivateAbilitiesOnSpawn();

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	void ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle);

	void RefreshRegenDelay(const FGameplayTag& DelayTag, float Duration);
	void ResetRegenDelays();

	FActiveGameplayEffectHandle StaminaRegenGEHandle;
	FActiveGameplayEffectHandle StaggerRegenGEHandle;
	FActiveGameplayEffectHandle StunRegenGEHandle;

	FActiveGameplayEffectHandle CombatStateEffectHandle;

};
