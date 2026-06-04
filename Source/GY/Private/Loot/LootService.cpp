#include "Loot/LootService.h"

#include "Enchant/EnchantOptionRoller.h"
#include "Engine/DataTable.h"
#include "Items/ItemDefinition.h"
#include "Loot/LootRows.h"
#include "Loot/RegionLootData.h"

namespace
{
	template <typename TRow>
	void GatherRows(UDataTable* Table, TArray<const TRow*>& OutRows)
	{
		if (!IsValid(Table)) return;

		Table->ForeachRow<TRow>(TEXT("LootService::GatherRows"),
			[&OutRows](const FName&, const TRow& Row)
			{
				if (Row.Weight > 0)
				{
					OutRows.Add(&Row);
				}
			});
	}

	// bRemove면 뽑은 행을 목록에서 제거 (without-replacement)
	template <typename TRow>
	const TRow* PickWeightedFromList(TArray<const TRow*>& Rows, FRandomStream& Stream, bool bRemove)
	{
		int32 TotalWeight = 0;
		for (const TRow* Row : Rows)
		{
			TotalWeight += Row->Weight;
		}
		if (TotalWeight <= 0) return nullptr;

		int32 Roll = Stream.RandRange(1, TotalWeight);
		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			Roll -= Rows[Index]->Weight;
			if (Roll <= 0)
			{
				const TRow* Picked = Rows[Index];
				if (bRemove)
				{
					Rows.RemoveAt(Index);
				}
				return Picked;
			}
		}
		return Rows.Num() > 0 ? Rows.Last() : nullptr;
	}

	template <typename TRow>
	const TRow* PickWeighted(UDataTable* Table, FRandomStream& Stream)
	{
		TArray<const TRow*> Rows;
		GatherRows<TRow>(Table, Rows);
		return PickWeightedFromList<TRow>(Rows, Stream, false);
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

	for (const FLootPoolEntry& Pool : Region->Pools)
	{
		if (!IsValid(Pool.ItemPool)) continue;

		// 보너스 풀 게이트
		if (Pool.DropChance < 1.f && Stream.FRand() > Pool.DropChance) continue;

		UDataTable* GradeTable = IsValid(Pool.GradeDistributionOverride) ? Pool.GradeDistributionOverride : Region->GradeDistribution;
		UDataTable* LevelTable = IsValid(Pool.LevelDistributionOverride) ? Pool.LevelDistributionOverride : Region->LevelDistribution;

		const int32 MinRolls = FMath::Min(Pool.MinRolls, Pool.MaxRolls);
		const int32 MaxRolls = FMath::Max(Pool.MinRolls, Pool.MaxRolls);
		const int32 Rolls = Stream.RandRange(MinRolls, MaxRolls);

		// without-replacement용 작업 후보 목록 (한 번만 모아두고 뽑을 때마다 제거)
		TArray<const FItemPoolRow*> WorkingRows;
		if (Pool.bDrawWithoutReplacement)
		{
			GatherRows<FItemPoolRow>(Pool.ItemPool, WorkingRows);
		}

		for (int32 RollIndex = 0; RollIndex < Rolls; ++RollIndex)
		{
			const FItemPoolRow* ItemRow = Pool.bDrawWithoutReplacement
				? PickWeightedFromList<FItemPoolRow>(WorkingRows, Stream, true)
				: PickWeighted<FItemPoolRow>(Pool.ItemPool, Stream);

			// without-replacement에서 후보 소진 시 중단
			if (ItemRow == nullptr) break;

			const FGradeDistributionRow* GradeRow = PickWeighted<FGradeDistributionRow>(GradeTable, Stream);
			const FLevelDistributionRow* LevelRow = PickWeighted<FLevelDistributionRow>(LevelTable, Stream);

			const int32 MinCount = FMath::Min(ItemRow->MinCount, ItemRow->MaxCount);
			const int32 MaxCount = FMath::Max(ItemRow->MinCount, ItemRow->MaxCount);

			// 같은 종류도 개별 슬롯으로 — 머지하지 않고 각 롤을 그대로 추가 (인벤 적재 시 스택)
			FLootDrop Drop;
			Drop.Definition = ItemRow->Definition;
			Drop.Count = Stream.RandRange(MinCount, MaxCount);
			Drop.GradeTag = GradeRow != nullptr ? GradeRow->GradeTag : FGameplayTag();
			Drop.Level = LevelRow != nullptr ? LevelRow->Level : 1;
			Drop.StatDeviation = RollStatDeviation(Stream);
			Drop.UsedSeed = Seed.GetInitialSeed();
			Drop.RolledOptions = EnchantOptionRoller::RollAllOptions(Drop.Definition.LoadSynchronous(), Drop.GradeTag, Stream);

			Result.Drops.Add(Drop);
		}
	}

	// TODO: PartySize 보정 (Context.PartySize로 풀별 Rolls/DropChance 스케일)
	return Result;
}
