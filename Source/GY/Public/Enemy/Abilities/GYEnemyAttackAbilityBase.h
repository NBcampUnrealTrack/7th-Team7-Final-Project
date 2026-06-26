#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "Enemy/DataTables/EnemyAbilityWeightRow.h"
#include "GYEnemyAttackAbilityBase.generated.h"

class UEnvQuery;

UENUM(BlueprintType)
enum class EGYEnemyAttackType : uint8
{
	None,
	Melee,
	Ranged,
};

UCLASS()
class GY_API UGYEnemyAttackAbilityBase : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGYEnemyAttackAbilityBase();

	bool CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const;

	float GetRemainingCooldown(const UAbilitySystemComponent* ASC) const;

	float GetTotalDamageScore() const;

	static float CalcAbilityScore(UGYEnemyAttackAbilityBase* Ability, const UAbilitySystemComponent* ASC,
		float DistToTarget, float AngleDeg, const UObject* LastUsed);

	void CanExecuteAbility();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	void RecalculateAttackDataFromMontage();
#endif
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void PlayAttackMontage();
	UFUNCTION()
	void OnMontageFinished();
	UFUNCTION()
	void OnMontageInterrupted();
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float AttackRange = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection",
		meta = (ClampMin = "0", ToolTip = "이 거리보다 가까우면 어빌리티 후보에서 제외. 0이면 비활성화"))
	float MinDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float AttackAngle = 360.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	EGYEnemyAttackType AttackType = EGYEnemyAttackType::None;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Selection")
	FName CalcSocket = TEXT("weapon_tip");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float BaseDamageScore = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	bool bHasCooldown = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Montage")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Montage")
	float PlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
	TArray<FHitDamageWeight> HitDamageWeights;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (Categories = "GameplayCue"))
	FGameplayTag HitCueTag; // 피격 시 카메라 이펙트

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (Categories = "GameplayCue"))
	FGameplayTag AttackCueTag; // 사운드 비주얼 FX 용

	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> EQSAsset;

	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> EQSCheckDistance;

	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> EQSCheckAngle;

	UPROPERTY(EditAnywhere, Category = "EQS|BB")
	float DistanceScore = 0.5f;

	UPROPERTY(EditAnywhere, Category = "EQS|BB")
	float AngleScore = 0.5f;
};
