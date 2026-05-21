#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GYEnemyAttackAbilityBase.generated.h"

UCLASS()
class GY_API UGYEnemyAttackAbilityBase : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGYEnemyAttackAbilityBase();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float AttackRange = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float BaseDamageScore = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	bool bHasCooldown = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Montage")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Montage")
	float PlayRate = 1.f;

	bool CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const;

	float GetRemainingCooldown(const UAbilitySystemComponent* ASC) const;
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageFinished();
	UFUNCTION()
	void OnMontageInterrupted();
};
