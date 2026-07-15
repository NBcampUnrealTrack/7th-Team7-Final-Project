#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GYEnemyAttackAbilityBase.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "TentacleStrike.generated.h"

class ATentacleActor;
class UAnimMontage;

UCLASS()
class GY_API UTentacleStrike : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()
public:
	UTentacleStrike();
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Tentacle")
	FTransform ResolveSpawnTransform() const;
	FTransform ResolveSpawnTransform_Implementation() const;

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	/** 촉수 몽타주의 MeleeTrace 노티파이에서 온 히트 이벤트 처리 */
	UFUNCTION()
	void OnTentacleHit(FGameplayEventData Payload);

private:
	void SpawnNextTentacle();
	void HandleTentacleMontageDone();
protected:
	/** 스폰할 촉수 액터 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle")
	TSubclassOf<ATentacleActor> TentacleClass;

	/** 촉수에 재생할 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle")
	TObjectPtr<UAnimMontage> TentacleMontage;

	/** 몽타주 재생 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.f;

	/** 스폰할 촉수 개수. */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle|Spawn", meta = (ClampMin = "1"))
	int32 TentacleCount = 8;

	/**
	 * 1번(첫 번째) 촉수의 로컬 위치 오프셋.
	 * XY 크기가 반경 역할을 하고, XY 방향이 시작 각도가 된다.
	 * Z 성분은 모든 촉수에 그대로 유지 (지면 높이 조정용).
	 *
	 * 예:
	 *   (0, -300, 0)  = 보스 왼쪽 300, 지면 높이 그대로
	 *   (300, 0, 0)   = 보스 정면 300
	 *   (0, 0, -50)   = XY 0 이면 스폰 불가 (경고 후 어빌리티 종료)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle|Spawn")
	FVector FirstLocalOffset = FVector(0.f, -300.f, 0.f);

	/**
	 * 반원을 어느 쪽으로 훑을지.
	 *   true  = 위(보스 앞쪽 X+) 반원을 지나감
	 *   false = 아래(보스 뒤쪽 X-) 반원을 지나감
	 * 반원(180도) 고정이라 마지막 촉수는 1번의 원 반대편에 옴.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle|Spawn")
	bool bSweepUpward = false;

	/** 각 스폰 사이 시간차(초). 0이면 동시에 스폰. */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle|Spawn", meta = (ClampMin = "0.0"))
	float SpawnInterval = 0.3f;

	/**
	 * 지정 시 보스 트랜스폼 대신 이 태그를 가진 레벨 액터의 트랜스폼을 스폰 기준으로 사용.
	 * 액터의 forward(X+)가 패턴의 정면 기준이 된다. 못 찾으면 보스 기준으로 폴백.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle|Spawn")
	FName AnchorActorTag = NAME_None;

	/** 몽타주 종료 후 Destroy 까지의 페이드아웃 시간(초). 0 이면 즉시 Destroy */
	UPROPERTY(EditDefaultsOnly, Category = "Tentacle", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.3f;

private:
	UPROPERTY()
	TArray<TObjectPtr<ATentacleActor>> SpawnedTentacle;

	int32 SpawnIndex = 0;
	int32 CompletedCount = 0;
	FTimerHandle SpawnTimerHandle;

	/** 활성화 시점에 1회 캐싱한 스폰 기준 트랜스폼. 패턴 도중 보스 이동/회전에 영향받지 않도록 함 */
	FTransform CachedSpawnTM;
};
