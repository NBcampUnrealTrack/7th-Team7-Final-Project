#pragma once

#include "CoreMinimal.h"
#include "Enchant/RolledEnchantOption.h"
#include "GameplayTagContainer.h"

class UItemDefinition;

namespace EnchantOptionRoller
{
	GY_API TArray<FRolledEnchantOption> RollOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream);

	GY_API TArray<FRolledEnchantOption> RollAllOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream);
}
