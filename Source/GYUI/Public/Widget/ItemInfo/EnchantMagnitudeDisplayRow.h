#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnchantMagnitudeDisplayRow.generated.h"

// MagnitudeTag별 툴팁 표기 정의. 행 키 = MagnitudeTag 전체 이름(예: "Enchant.Magnitude.LightAttackDamagePct").
USTRUCT(BlueprintType)
struct FEnchantMagnitudeDisplayRow : public FTableRowBase
{
	GENERATED_BODY()

	// {0}에 반올림한 롤값이 들어감. 예: "약공격 피해 +{0}%"
	UPROPERTY(EditAnywhere)
	FText Format;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 3))
	int32 Decimals = 0;
};
