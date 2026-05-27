#include "Loot/LootService.h"

#include "Enchant/EnchantOptionRoller.h"
#include "Engine/DataTable.h"
#include "Items/ItemDefinition.h"
#include "Loot/LootRows.h"
#include "Loot/RegionLootData.h"

namespace
{
	template <typename TRow>
	const TRow* PickWeighted(UDataTable* Table, FRandomStream& Stream)
	{
		if (!IsValid(Table)) return nullptr;

		TArray<const TRow*> Candidates;
		int32 TotalWeight = 0;

		Table->ForeachRow<TRow>(TEXT("LootService::PickWeighted"),
			[&Candidates, &TotalWeight](const FName&, const TRow& Row)
			{
				if (Row.Weight > 0)
				{
					Candidates.Add(&Row);
					TotalWeight += Row.Weight;
				}
			});

		if (TotalWeight <= 0) return nullptr;

		int32 Roll = Stream.RandRange(1, TotalWeight);
		for (const TRow* Row : Candidates)
		{
			Roll -= Row->Weight;
			if (Roll <= 0) return Row;
		}
		return Candidates.Last();
	}

	float RollStatDeviation(FRandomStream& Stream)
	{
		return Stream.FRandRange(-0.05f, 0.05f);
	}
}

FLootResult ULootService::RollLoot(const URegionLootData* Region, const FLootContext& Context, const FRandomStream& Seed) const
{
	FLootResult Result;
	if (!IsValid(Region)) return Result;

	FRandomStream Stream = Seed;

	const FItemPoolRow* ItemRow = PickWeighted<FItemPoolRow>(Region->ItemPool, Stream);
	const FGradeDistributionRow* GradeRow = PickWeighted<FGradeDistributionRow>(Region->GradeDistribution, Stream);
	const FLevelDistributionRow* LevelRow = PickWeighted<FLevelDistributionRow>(Region->LevelDistribution, Stream);

	if (ItemRow == nullptr || GradeRow == nullptr || LevelRow == nullptr) return Result;

	FLootDrop Drop;
	Drop.Definition = ItemRow->Definition;
	Drop.Count = Stream.RandRange(ItemRow->MinCount, ItemRow->MaxCount);
	Drop.GradeTag = GradeRow->GradeTag;
	Drop.Level = LevelRow->Level;
	Drop.StatDeviation = RollStatDeviation(Stream);
	Drop.UsedSeed = Seed.GetInitialSeed();
	Drop.RolledOptions = EnchantOptionRoller::RollAllOptions(Drop.Definition.LoadSynchronous(), Drop.GradeTag, Stream);

	// TODO: PartySize 보정

	Result.Drops.Add(Drop);
	return Result;
}
