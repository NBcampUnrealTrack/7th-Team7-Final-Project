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
