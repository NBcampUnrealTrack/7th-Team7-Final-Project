#pragma once

#include "CoreMinimal.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "EnemyTeleport.generated.h"

/**
 * 잡몹용 텔레포트 어빌리티 (몽타주 + VFX 연출).
 *
 * 흐름:
 *  1. AttackMontage 재생 시작 + DisappearCue 발사 (Exit 모션 = 사라지는 연출)
 *  2. 거리에 비례해 PlayRate 조절 → 멀수록 더 오래 사라져 있음
 *  3. 몽타주 중간 AnimNotify 가 TeleportTriggerEventTag 이벤트 발사 → SetActorLocation + AppearCue
 *  4. 몽타주 남은 부분(Enter 모션 = 나타나는 연출, Exit 역재생 등) 자연스럽게 진행
 *  5. 몽타주 종료 시 EndAbility
 *
 * 디자이너는 단일 AttackMontage 안에 [Exit → TeleportPoint Notify → Enter] 흐름을 만들면 됨.
 */
UCLASS()
class GY_API UEnemyTeleport : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()

public:
	UEnemyTeleport();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnTeleportPointReached(FGameplayEventData Payload);

	UFUNCTION()
	void OnTeleportMontageFinished();

	UFUNCTION()
	void OnTeleportMontageInterrupted();

	bool ResolveDestination(FVector& OutLocation, AActor*& OutTarget) const;

	/** 타겟에서 떨어진 거리만큼 떨어진 자리로 텔포 */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (ClampMin = "0.0"))
	float DistanceFromTarget = 200.f;

	/** 텔포 후 타겟 향해 회전 */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	bool bFaceTargetAfterTeleport = true;

	/** AnimNotify 에서 발사할 텔포 트리거 이벤트 태그 (사라진 시점) */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Trigger", meta = (Categories = "Event"))
	FGameplayTag TeleportTriggerEventTag;

	/** 거리(cm)당 추가될 몽타주 재생 시간(초). 멀수록 사라져 있는 시간이 길어짐. */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Timing", meta = (ClampMin = "0.0"))
	float TimePerUnitDistance = 0.001f;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Timing", meta = (ClampMin = "0.0"))
	float MinMontageDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Timing", meta = (ClampMin = "0.0"))
	float MaxMontageDuration = 2.0f;

	/** 어빌리티 시작 시 발사할 큐 (사라지는 VFX) */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag DisappearCueTag;

	/** 텔포 트리거 시점에 도착 위치에서 발사할 큐 (나타나는 VFX) */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport|Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag AppearCueTag;

private:
	FVector PendingDestination = FVector::ZeroVector;
	TWeakObjectPtr<AActor> PendingTarget;
};
