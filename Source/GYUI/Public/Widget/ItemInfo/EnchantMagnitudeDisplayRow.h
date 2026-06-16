#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnchantMagnitudeDisplayRow.generated.h"

// MagnitudeTag별 툴팁 표기 포맷. 행 키 = MagnitudeTag 전체 이름(예: "Enchant.Magnitude.DamageDealtPct").
// 자리수는 롤 시점에 이미 양자화되므로 여기선 포맷 문구만 보유.
USTRUCT(BlueprintType)
struct FEnchantMagnitudeDisplayRow : public FTableRowBase
{
	GENERATED_BODY()

	// {0}에 롤값이 들어감. 예: "약공격 피해 +{0}%"
	UPROPERTY(EditAnywhere)
	FText Format;
};
