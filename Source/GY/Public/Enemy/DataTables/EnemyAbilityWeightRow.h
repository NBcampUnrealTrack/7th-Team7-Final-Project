#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyAbilityWeightRow.generated.h"

class UGYEnemyAttackAbilityBase;

USTRUCT(BlueprintType)
struct FHitDamageWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Additive = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Multiplicative = 1.f;

	/** 이 타격이 가하는 경직(Stagger) 게이지 누적량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Stagger = 0.f;

	/** 이 타격이 가하는 무력(Stun) 게이지 누적량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Stun = 0.f;
};


USTRUCT(BlueprintType)
struct FEnemyAbilityWeightRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<UGYEnemyAttackAbilityBase> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FHitDamageWeight> HitDamageWeights;
};
