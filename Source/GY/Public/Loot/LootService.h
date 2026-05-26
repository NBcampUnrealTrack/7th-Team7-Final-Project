#pragma once

#include "CoreMinimal.h"
#include "Loot/LootTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LootService.generated.h"

class URegionLootData;

UCLASS()
class GY_API ULootService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	FLootResult RollLoot(const URegionLootData* Region, const FLootContext& Context, const FRandomStream& Seed) const;
};
