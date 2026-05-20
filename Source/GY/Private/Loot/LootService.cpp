#include "Loot/LootService.h"

#include "Enchant/EnchantOptionRoller.h"
#include "Engine/DataTable.h"
#include "Loot/LootTableRow.h"

namespace
{
	TArray<const FLootTableRow*> GatherCandidates(UDataTable* LootTable, FName SourceId)
	{
		TArray<const FLootTableRow*> Result;
		if (!IsValid(LootTable)) return Result;

		LootTable->ForeachRow<FLootTableRow>(TEXT("LootService::RollLoot"),
			[&Result, SourceId](const FName& RowName, const FLootTableRow& Row)
			{
				if (Row.SourceId == SourceId && Row.Weight > 0)
				{
					Result.Add(&Row);
				}
			});

		return Result;
	}

	const FLootTableRow* PickWeightedCandidate(const TArray<const FLootTableRow*>& Candidates, FRandomStream& Stream)
	{
		int32 TotalWeight = 0;
		for (const FLootTableRow* Row : Candidates)
		{
			TotalWeight += Row->Weight;
		}
		if (TotalWeight <= 0) return nullptr;

		int32 Roll = Stream.RandRange(1, TotalWeight);
		for (const FLootTableRow* Row : Candidates)
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

FLootResult ULootService::RollLoot(const FLootContext& Context, UDataTable* LootTable, const FRandomStream& Seed) const
{
	FLootResult Result;

	const TArray<const FLootTableRow*> Candidates = GatherCandidates(LootTable, Context.SourceId);
	if (Candidates.IsEmpty()) return Result;

	FRandomStream Stream = Seed;

	const FLootTableRow* Picked = PickWeightedCandidate(Candidates, Stream);
	if (Picked == nullptr) return Result;

	FLootDrop Drop;
	Drop.Definition = Picked->Definition;
	Drop.Count = Stream.RandRange(Picked->MinCount, Picked->MaxCount);
	Drop.GradeTag = Picked->GradeTag;
	Drop.Level = Picked->Level;
	Drop.StatDeviation = RollStatDeviation(Stream);
	Drop.UsedSeed = Seed.GetInitialSeed();
	Drop.RolledOptionIds = EnchantOptionRoller::RollAllOptions(Drop.Definition.LoadSynchronous(), Drop.GradeTag, Stream);

	// TODO: CT_RegionScaling으로 Grade/Level 결정
	// TODO: PartySize 보정

	Result.Drops.Add(Drop);
	return Result;
}
