#include "Loot/LootService.h"

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

	FGameplayTag PickGrade(const FGameplayTagContainer& AllowedGrades, FRandomStream& Stream)
	{
		const int32 Num = AllowedGrades.Num();
		if (Num == 0) return FGameplayTag();
		const int32 Index = Stream.RandRange(0, Num - 1);

		int32 Cursor = 0;
		for (const FGameplayTag& Tag : AllowedGrades)
		{
			if (Cursor == Index) return Tag;
			++Cursor;
		}
		return FGameplayTag();
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
	Drop.GradeTag = PickGrade(Picked->AllowedGrades, Stream);
	Drop.StatDeviation = RollStatDeviation(Stream);
	Drop.UsedSeed = Seed.GetInitialSeed();

	// TODO: RegionLevel에 따른 등급 분포 (CT_RegionScaling)
	// TODO: EnchantOption 풀 롤 → RolledOptionIds 채움
	// TODO: Grade == Legendary 시 Penalty 자동 부여
	// TODO: PartySize 보정

	Result.Drops.Add(Drop);
	return Result;
}
