#pragma once

#include "Engine/DataTable.h"
#include "GYPlayerBaseStatsRow.generated.h"

// 플레이어 base 어트리뷰트 초기값. DT_PlayerBaseStats의 행 구조체.
USTRUCT(BlueprintType)
struct FGYPlayerBaseStatsRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base", meta = (ClampMin = "0.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base", meta = (ClampMin = "0.0"))
	float Attack = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base", meta = (ClampMin = "0.0"))
	float Defense = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Strength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Dexterity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additional", meta = (ClampMin = "0.0"))
	float MaxStagger = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additional", meta = (ClampMin = "0.0"))
	float MaxStun = 100.f;
};
