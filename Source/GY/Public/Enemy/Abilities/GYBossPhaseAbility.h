#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GYBossPhaseAbility.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSubAbilityFinished, TSubclassOf<UGameplayAbility>, AbilityClass);

UCLASS()
class GY_API UGYBossPhaseAbility : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGYBossPhaseAbility();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|SubAbility")
	bool ActivateSubAbility(TSubclassOf<UGameplayAbility> AbilityClass);

	UPROPERTY(BlueprintAssignable, Category = "Boss|Phase|SubAbility")
	FOnSubAbilityFinished OnSubAbilityFinished;

	void CachingParticipants();

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase")
	void ApplyPhaseEntry();

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase")
	void ApplyPhaseExit();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Phase")
	void OnPhaseExecute();

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase")
	void FinishPhase();

	UFUNCTION(BlueprintPure, Category = "Boss|Phase|Participants")
	TArray<APlayerState*> GetCachedParticipants() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Phase|Participants")
	int32 GetCachedParticipantCount() const { return CachedParticipants.Num(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Phase|Participants")
	TArray<APawn*> GetCachedParticipantPawns() const;

	UFUNCTION(BlueprintCallable, Category ="Boss|Phase|Participants")
	void ApplyGEToAllParticipants(TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Participants")
	void ExecuteCueOnAllParticipants(FGameplayTag CueTag);

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Participants")
	void ApplyGEToParticipant(APlayerState* Target, TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Self")
	void GrantAbilitiesNow();

	UFUNCTION(BlueprintCallable, Category = "Boss|Phase|Self")
	void RemoveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& Abilities);
private:
	UFUNCTION()
	void HandleSubAbilityEnded(UGameplayAbility* Ability);
protected:
	/** 진입 시 부여할 무적/슈퍼아머 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry")
	FGameplayTagContainer InvulnerabilityTags;

	/** 진입 시 캔슬할 진행 중 어빌리티 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry")
	FGameplayTagContainer CancelAbilitiesWithTags;

	/** 진입 시 보스에게 적용할 영구 GE (예 : 스탯 강화 ) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry")
	TArray<TSubclassOf<UGameplayEffect>> EntryGameplayEffects;

	/** 진입 시 모든 참가자에게 적용할 즉시성 GE (예 : 광역 데미지) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry")
	TArray<TSubclassOf<UGameplayEffect>> EntryParticipantGameplayEffects;

	/** 진입 시 보스 ASC에서 발사할 Cue */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry")
	FGameplayTag EntryCueTag;

	/** 진입 시 각 참가자 ASC에서 발사할 Cue (화면 흔들림 등등)*/
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Entry")
	FGameplayTag EntryParticipantCueTag;

	/** 종료 시 Grant할 새 GA (페이즈별 젼용 패턴) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Exit")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrantOnExit;

	/** 종료 시 회수 할 GA */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Exit")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToRemoveOnExit;

	/** 종료 시 보스 ASC에서 발사할 Cue */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase|Exit")
	FGameplayTag ExitCueTag;

protected:
	/** ActivateAbility 시점의 참가자 스냅샷 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerState>> CachedParticipants;

	/** Entry에서 적용한 영구 GE Handle (Cancel/EndAbility 시 회수용) */
	UPROPERTY(Transient)
	TArray<FActiveGameplayEffectHandle> ActiveEntryEffectHandles;

	/** Exit에서 Grant할 GA Spec Handle */
	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;

	/** 페이즈 동안 Grant한 임시 GA 핸들 */
	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> TemporaryGrantedHandles;

	bool bPhaseFinished = false;

};

