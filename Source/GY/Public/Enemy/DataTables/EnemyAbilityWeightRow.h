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
