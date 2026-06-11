#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EnchantOptionRow.generated.h"

// 옵션이 부여하는 개별 수치. 한 옵션이 여러 개 가질 수 있음(예: 묵직함 = 강공 피해 + 무력화 차감).
// MagnitudeTag로 적용 GE는 DT_EnchantMagnitudeEffect, 표기 문구는 DT_EnchantMagnitudeDisplay에서 조회.
USTRUCT(BlueprintType)
struct GY_API FEnchantMagnitudeDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Enchant.Magnitude"))
	FGameplayTag MagnitudeTag;

	UPROPERTY(EditAnywhere)
	float Min = 0.f;

	UPROPERTY(EditAnywhere)
	float Max = 0.f;

	// 롤·표기 자리수(단일 소스). 롤값을 이 자리수로 반올림 → 툴팁 표기와 항상 일치
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 3))
	int32 Decimals = 0;
};

USTRUCT(BlueprintType)
struct GY_API FEnchantOptionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Equipment.Slot"))
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere)
	FGameplayTag AffinityTag;

	UPROPERTY(EditAnywhere)
	FGameplayTag TriggerCondition;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 RollWeight = 1;

	// 옵션이 부여하는 수치 정의(1~N개). 롤 시 각 항목의 Min~Max를 굴림.
	UPROPERTY(EditAnywhere)
	TArray<FEnchantMagnitudeDef> Magnitudes;

	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere, meta = (MultiLine = true))
	FText Description;
};
