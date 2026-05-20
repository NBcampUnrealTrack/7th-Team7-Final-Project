#pragma once

#include "CoreMinimal.h"

struct FEnchantOptionRow;
class UItemDefinition;

namespace EnchantOptionResolver
{
	GY_API const FEnchantOptionRow* FindRow(UItemDefinition* Def, FName OptionId);
}
