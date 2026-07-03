#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "ArenaImpactAbility.generated.h"

class AAreaImpactProjectile;

/**
 * 아레나 중앙 고정 지점에 큰 투사체를 수직 낙하시켜 광역 타격하는 어빌리티.
 * 페이즈 실패(MinionGate 타임아웃) 시 벌칙으로 발동된다.
 * 경고 표시는 GYBossPhaseAbility 가 게이트 시작 시 미리 스폰/성장시키므로,
 * 이 어빌리티는 낙하 타격만 담당한다.
 */
UCLASS()
class GY_API UArenaImpactAbility : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnProjectileHit(FGameplayEventData Payload);

protected:
	/** 아레나 중앙에 낙하시킬 투사체 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ArenaImpact")
	TSubclassOf<AAreaImpactProjectile> ProjectileClass;

	/** 아레나 중앙 지점으로 사용할 레벨 액터 태그 (워닝과 동일 값 권장) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ArenaImpact")
	FName ArenaCenterTag = TEXT("ArenaCenter");

	/** 투사체 스폰 높이 (아레나 중앙 지면 기준 +Z) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ArenaImpact")
	float SpawnHeight = 2000.f;

	/** 투사체 낙하 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack|ArenaImpact")
	float DropSpeed = 3000.f;
};
