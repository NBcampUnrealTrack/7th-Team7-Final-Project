#pragma once

#include "Engine/DataTable.h"
#include "EnemyStatRow.generated.h"

USTRUCT(BlueprintType)
struct FEnemyStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackPower = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Defense = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MoveSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Additional")
	float MaxStagger = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Additional")
	float MaxStun = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Additional")
	float CriticalRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Additional")
	float CriticalMultiplier = 1.5f;
};
