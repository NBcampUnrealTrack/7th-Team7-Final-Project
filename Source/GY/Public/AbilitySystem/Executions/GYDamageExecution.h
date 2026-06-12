#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GYDamageExecution.generated.h"

// 순수 HP 데미지 계산. 공격자 Attack/Crit/STR/DEX + 대상 DEF 캡처, SetByCaller 모션배율로 산출.
// 경직/무력(poise)은 여기서 다루지 않음 — 호출부에서 별도 적용.
UCLASS()
class GY_API UGYDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UGYDamageExecution();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
