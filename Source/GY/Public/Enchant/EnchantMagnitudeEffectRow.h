#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Templates/SubclassOf.h"
#include "EnchantMagnitudeEffectRow.generated.h"

class UGameplayEffect;

// MagnitudeTag별 적용 GE. 행 키 = MagnitudeTag 전체 이름(예: "Enchant.Magnitude.StaggerResist").
// 매핑 없는 매그니튜드(조건부·프록)는 장착 시 어트리뷰트로 적용하지 않음.
USTRUCT(BlueprintType)
struct GY_API FEnchantMagnitudeEffectRow : public FTableRowBase
{
	GENERATED_BODY()

	// 단일 모디파이어 프리미티브 GE. 값은 공용 SetByCaller 키로 주입됨.
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> Effect;

	// 롤값에 곱해 GE에 주입(예: 퍼센트 10 → 비율 0.1이면 0.01).
	UPROPERTY(EditAnywhere)
	float ValueScale = 1.f;
};
