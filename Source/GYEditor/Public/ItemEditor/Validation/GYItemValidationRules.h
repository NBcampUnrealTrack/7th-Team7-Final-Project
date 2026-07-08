#pragma once

#include "CoreMinimal.h"
#include "ItemEditor/Validation/GYItemValidationTypes.h"

struct FGYItemValidationContext;
class UItemDefinition;

class IGYItemValidationRule
{
public:
	virtual ~IGYItemValidationRule() = default;

	virtual void Validate(
		const FGYItemValidationContext& Context,
		const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const = 0;
};

TArray<TUniquePtr<IGYItemValidationRule>> MakeDefaultItemValidationRules();

// 저장 시 자동 검증용 서브셋 — 풀/Region 전수 스캔이 필요한 규칙 제외
TArray<TUniquePtr<IGYItemValidationRule>> MakeSaveTimeItemValidationRules();
