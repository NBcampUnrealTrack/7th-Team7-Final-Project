#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "Enemy/DataTables/EnemyAbilityWeightRow.h"
#include "GYEnemyAttackAbilityBase.generated.h"

UCLASS()
class GY_API UGYEnemyAttackAbilityBase : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGYEnemyAttackAbilityBase();

	bool CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const;

	float GetRemainingCooldown(const UAbilitySystemComponent* ASC) const;

	virtual void FaceTarget();

	float GetTotalDamageScore() const;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float AttackAngle = 360.f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
	TArray<FHitDamageWeight> HitDamageWeights;
};
