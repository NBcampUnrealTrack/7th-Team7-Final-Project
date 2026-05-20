#include "Loot/LootService.h"

#include "Core/GameplayTags/ItemTags.h"
#include "Enchant/EnchantSlotPolicy.h"
#include "Enchant/GYEnchantSettings.h"
#include "Engine/DataTable.h"
#include "Items/EnchantOptionRow.h"
#include "Items/Fragments/ItemFragment_Enchantable.h"
#include "Items/ItemDefinition.h"
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

	struct FOptionCandidate
	{
		FName Id;
		int32 Weight = 0;
	};

	TArray<FOptionCandidate> GatherOptionCandidates(UDataTable* Pool)
	{
		TArray<FOptionCandidate> Result;
		if (!IsValid(Pool)) return Result;

		Pool->ForeachRow<FEnchantOptionRow>(TEXT("LootService::RollOptions"),
			[&Result](const FName& RowName, const FEnchantOptionRow& Row)
			{
				if (Row.RollWeight > 0)
				{
					Result.Add({RowName, Row.RollWeight});
				}
			});
		return Result;
	}

	FName PickWeightedOption(const TArray<FOptionCandidate>& Candidates, FRandomStream& Stream)
	{
		int32 TotalWeight = 0;
		for (const FOptionCandidate& Candidate : Candidates)
		{
			TotalWeight += Candidate.Weight;
		}
		if (TotalWeight <= 0) return NAME_None;

		int32 Roll = Stream.RandRange(1, TotalWeight);
		for (const FOptionCandidate& Candidate : Candidates)
		{
			Roll -= Candidate.Weight;
			if (Roll <= 0) return Candidate.Id;
		}
		return Candidates.Last().Id;
	}

	TArray<FName> RollEnchantOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream)
	{
		TArray<FName> Result;
		if (!IsValid(Def)) return Result;

		const int32 SlotCount = EnchantSlotPolicy::GetBonusSlotCount(GradeTag);
		if (SlotCount <= 0) return Result;

		const UItemFragment_Enchantable* Fragment = Def->FindFragment<UItemFragment_Enchantable>();
		if (Fragment == nullptr) return Result;

		UDataTable* Pool = Fragment->EnchantOptionPoolTable.LoadSynchronous();
		if (!IsValid(Pool)) return Result;

		TArray<FOptionCandidate> Candidates = GatherOptionCandidates(Pool);

		for (int32 i = 0; i < SlotCount; ++i)
		{
			if (Candidates.IsEmpty()) break;
			const FName Picked = PickWeightedOption(Candidates, Stream);
			if (Picked.IsNone()) break;
			Result.Add(Picked);
			Candidates.RemoveAll([&Picked](const FOptionCandidate& Candidate)
			{
				return Candidate.Id == Picked;
			});
		}

		return Result;
	}

	FName RollPenaltyOption(FRandomStream& Stream)
	{
		const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();
		if (!IsValid(Settings)) return NAME_None;

		UDataTable* PenaltyPool = Settings->PenaltyOptionTable.LoadSynchronous();
		if (!IsValid(PenaltyPool)) return NAME_None;

		const TArray<FOptionCandidate> Candidates = GatherOptionCandidates(PenaltyPool);
		return PickWeightedOption(Candidates, Stream);
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
	Drop.RolledOptionIds = RollEnchantOptions(Drop.Definition.LoadSynchronous(), Drop.GradeTag, Stream);

	if (Drop.GradeTag.MatchesTagExact(GYGameplayTags::Item_Grade_Legendary_Engraved))
	{
		const FName PenaltyId = RollPenaltyOption(Stream);
		if (!PenaltyId.IsNone()) Drop.RolledOptionIds.Add(PenaltyId);
	}

	// TODO: CT_RegionScaling으로 Grade/Level 결정
	// TODO: PartySize 보정

	Result.Drops.Add(Drop);
	return Result;
}
