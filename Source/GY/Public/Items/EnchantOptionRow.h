#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "EnchantOptionRow.generated.h"

class UGameplayEffect;

// 옵션이 부여하는 개별 수치. 한 옵션이 여러 개 가질 수 있음(예: 묵직함 = 강공 피해 + 무력화 차감).
// 표기 문구/소수자리는 MagnitudeTag로 DT_EnchantMagnitudeDisplay에서 조회(위젯).
USTRUCT(BlueprintType)
struct GY_API FEnchantMagnitudeDef
{
	GENERATED_BODY()

	// TemplateGE의 SetByCaller 키
	UPROPERTY(EditAnywhere, meta = (Categories = "Enchant.Magnitude"))
	FGameplayTag MagnitudeTag;

	UPROPERTY(EditAnywhere)
	float Min = 0.f;

	UPROPERTY(EditAnywhere)
	float Max = 0.f;
};

USTRUCT(BlueprintType)
struct GY_API FEnchantOptionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Equipment.Slot"))
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> TemplateGE;

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
