#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UItemDefinition;

namespace EnchantOptionRoller
{
	GY_API TArray<FName> RollOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream);

	GY_API FName RollPenalty(FRandomStream& Stream);

	GY_API TArray<FName> RollAllOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream);
}
