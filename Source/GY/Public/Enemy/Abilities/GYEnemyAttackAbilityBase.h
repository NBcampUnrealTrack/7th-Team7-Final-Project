#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GYEnemyAttackAbilityBase.generated.h"

UCLASS()
class GY_API UGYEnemyAttackAbilityBase : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float AttackRange = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	float BaseDamageScore = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Selection")
	bool bHasCooldown = false;

	bool CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const;

	float GetRemainingCooldown(const UAbilitySystemComponent* ASC) const;
};
