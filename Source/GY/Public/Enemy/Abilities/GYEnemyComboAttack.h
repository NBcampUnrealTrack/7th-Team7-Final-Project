#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "GYEnemyComboAttack.generated.h"

class UAbilityTask_PlayMontageAndWait;
class AProjectileBase;

USTRUCT(BlueprintType)
struct FComboStep
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float AttackRange = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float MinDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float AttackAngle = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TArray<FHitDamageWeight> HitWeights;

	// true면 이 스텝 몽타주의 LaunchProjectile 노티 시점에 검기 발사
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile")
	bool bLaunchProjectile = false;

	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile",
		meta = (EditCondition = "bLaunchProjectile", EditConditionHides))
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile",
		meta = (EditCondition = "bLaunchProjectile", EditConditionHides))
	float ProjectileSpeed = 1500.f;

	// 발사 위치로 쓸 검 Actor 슬롯. 못 찾으면 SkeletalMesh의 CalcSocket에서 발사
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile",
		meta = (EditCondition = "bLaunchProjectile", EditConditionHides))
	FGameplayTag WeaponSlotTag;

	// 검 StaticMesh의 소켓. None이면 검 Actor 원점
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile",
		meta = (EditCondition = "bLaunchProjectile", EditConditionHides))
	FName WeaponSocket;

	// true면 바닥에 붙어서 나감(바닥 위 GroundHeightOffset 높이, 수평 직진), false면 소켓 위치에서 나감
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile",
		meta = (EditCondition = "bLaunchProjectile", EditConditionHides))
	bool bLaunchFromGround = false;

	// 바닥에서 띄울 높이
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile",
		meta = (EditCondition = "bLaunchProjectile && bLaunchFromGround", EditConditionHides))
	float GroundHeightOffset = 30.f;

	// true면 타겟 몸통 중심으로 높이 보정, false면 발사 높이 그대로 수평 직진 (소켓 모드에서만)
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile",
		meta = (EditCondition = "bLaunchProjectile && !bLaunchFromGround", EditConditionHides))
	bool bAimAtTargetCenter = true;

	// 검기 모양의 Z축(Yaw) 회전 오프셋(도). 진행 방향 기준으로 더해지며 날아가는 방향은 안 바뀜
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Projectile",
		meta = (EditCondition = "bLaunchProjectile", EditConditionHides))
	float SlashYawOffset = 0.f;
};

UCLASS()
class GY_API UGYEnemyComboAttack : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Combo")
	TArray<FComboStep> ComboSteps;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual const FHitDamageWeight* GetCurrentHitWeight() const override;

	// flat 테이블 배열을 [AttackMontage 몫 → ComboSteps 순서]로 분배 (GYEditor 동기화와 동일 규칙)
	virtual void ApplyWeightRow(const FEnemyAbilityWeightRow& Row) override;

	virtual bool ShouldContinueCombo() const;

	UFUNCTION()
	void OnComboBranch(FGameplayEventData Payload);

	UFUNCTION()
	void OnLaunchProjectile(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboMontageEnded();

	UFUNCTION()
	void OnComboMontageInterrupted();

	void PlayComboMontage(int32 Index);

	int32 ComboIndex = 0;

	UPROPERTY(EditDefaultsOnly, Category="Combat|Selection")
	bool bFlying = false;

private:

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;
};
