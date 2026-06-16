#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Templates/SubclassOf.h"
#include "EnchantMagnitudeEffectRow.generated.h"

class UGameplayEffect;

// MagnitudeTag별 장착 시 적용 GE. 행 키 = MagnitudeTag 전체 이름.
// 행 없는 매그니튜드는 GE 적용 안 함(타격 시점 값배율로 OnHitModifier에만 등록됨).
USTRUCT(BlueprintType)
struct GY_API FEnchantMagnitudeEffectRow : public FTableRowBase
{
	GENERATED_BODY()

	// 장착 시 self에 적용할 GE. 스탯=어트리뷰트 가산 GE, 온히트 효과=로직 주입 GE. 값은 공용 SetByCaller 키로 주입.
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> Effect;

	// 롤값에 곱해 GE에 주입(예: 퍼센트 10 → 비율 0.1이면 0.01).
	UPROPERTY(EditAnywhere)
	float ValueScale = 1.f;

	// true면 GE 적용과 별개로 OnHitModifier에도 등록(타격 시점에 값이 필요한 온히트 효과: 출혈량 등).
	UPROPERTY(EditAnywhere)
	bool bHitTimeValue = false;
};
