#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "GYEnemyComboAttack.generated.h"

class UAbilityTask_PlayMontageAndWait;

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

	virtual const FHitDamageWeight* GetCurrentHitWeight() const override;

	// flat 테이블 배열을 [AttackMontage 몫 → ComboSteps 순서]로 분배 (GYEditor 동기화와 동일 규칙)
	virtual void ApplyWeightRow(const FEnemyAbilityWeightRow& Row) override;

	virtual bool ShouldContinueCombo() const;

	UFUNCTION()
	void OnComboBranch(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboMontageEnded();

	UFUNCTION()
	void OnComboMontageInterrupted();

	void PlayComboMontage(int32 Index);

	int32 ComboIndex = 0;

private:

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;
};
