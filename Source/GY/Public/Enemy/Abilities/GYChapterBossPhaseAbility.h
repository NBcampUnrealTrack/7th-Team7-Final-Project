#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GYChapterBossPhaseAbility.generated.h"

class ULevelSequence;

UCLASS()
class GY_API UGYChapterBossPhaseAbility : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGYChapterBossPhaseAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	void OnWeaponSwapTime();
	void OnPhaseEnd();

	/** 시네마틱 */
	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	TSoftObjectPtr<ULevelSequence> CinematicSequence;

	/** 시네마틱 시작 후 무기/애님 스왑까지의 지연(초) — 화면이 가려진 타이밍 */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = "0.0"))
	float WeaponSwapDelay = 2.f;

	/** 페이즈 전체 길이(초) = 시네마틱 길이. */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = "0.0"))
	float PhaseDuration = 6.f;

	/** 페이즈 동안 부여할 무적 태그 (종료 시 자동 제거) */
	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	FGameplayTagContainer InvulnerabilityTags;

	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	FGameplayTagContainer PhaseTagsToAdd;

	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToRemove;   // 대검 패턴

	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;    // 장검 패턴

	FTimerHandle SwapTimer;
	FTimerHandle EndTimer;
};
