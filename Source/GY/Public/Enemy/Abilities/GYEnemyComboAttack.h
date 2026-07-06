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
	virtual bool ShouldContinueCombo() const;

	UFUNCTION()
	void OnComboBranch(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboMontageEnded();

	UFUNCTION()
	void OnComboMontageInterrupted();

	void PlayComboMontage(int32 Index);

private:
	int32 ComboIndex = 0;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;
};
