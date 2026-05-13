#pragma once

#include "CoreMinimal.h"
#include "Loot/LootTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LootService.generated.h"

class UDataTable;

UCLASS()
class GY_API ULootService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	FLootResult RollLoot(const FLootContext& Context, UDataTable* LootTable, const FRandomStream& Seed) const;
};
