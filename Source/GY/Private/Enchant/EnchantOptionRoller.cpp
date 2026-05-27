#include "Enchant/EnchantOptionRoller.h"

#include "Core/GameplayTags/ItemTags.h"
#include "Enchant/EnchantSlotPolicy.h"
#include "Enchant/GYEnchantSettings.h"
#include "Engine/DataTable.h"
#include "Items/EnchantMagnitudeRow.h"
#include "Items/EnchantOptionRow.h"
#include "Items/Fragments/ItemFragment_Enchantable.h"
#include "Items/ItemDefinition.h"

namespace
{
	struct FRollCandidate
	{
		FName Id;
		int32 Weight = 0;
	};

	TArray<FRollCandidate> GatherCandidates(UDataTable* Pool)
	{
		TArray<FRollCandidate> Result;
		if (!IsValid(Pool)) return Result;

		Pool->ForeachRow<FEnchantOptionRow>(TEXT("EnchantOptionRoller::Gather"),
			[&Result](const FName& RowName, const FEnchantOptionRow& Row)
			{
				if (Row.RollWeight > 0)
				{
					Result.Add({RowName, Row.RollWeight});
				}
			});
		return Result;
	}

	FName PickWeighted(const TArray<FRollCandidate>& Candidates, FRandomStream& Stream)
	{
		int32 TotalWeight = 0;
		for (const FRollCandidate& Candidate : Candidates)
		{
			TotalWeight += Candidate.Weight;
		}
		if (TotalWeight <= 0) return NAME_None;

		int32 Roll = Stream.RandRange(1, TotalWeight);
		for (const FRollCandidate& Candidate : Candidates)
		{
			Roll -= Candidate.Weight;
			if (Roll <= 0) return Candidate.Id;
		}
		return Candidates.Last().Id;
	}

	// OptionId에 매칭되는 magnitude 행들을 롤해서 FRolledEnchantOption 구성.
	// bUseMax면 Max 고정, 아니면 RandRange(Min, Max) 균등.
	FRolledEnchantOption RollMagnitudes(FName OptionId, bool bUseMax, FRandomStream& Stream)
	{
		FRolledEnchantOption Result;
		Result.OptionId = OptionId;

		const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();
		if (!IsValid(Settings)) return Result;

		UDataTable* MagnitudeTable = Settings->EnchantMagnitudeTable.LoadSynchronous();
		if (!IsValid(MagnitudeTable)) return Result;

		MagnitudeTable->ForeachRow<FEnchantMagnitudeRow>(TEXT("EnchantOptionRoller::RollMagnitudes"),
			[&Result, OptionId, bUseMax, &Stream](const FName&, const FEnchantMagnitudeRow& Row)
			{
				if (Row.OptionId != OptionId) return;

				FRolledMagnitude Magnitude;
				Magnitude.MagnitudeTag = Row.MagnitudeTag;
				Magnitude.Value = bUseMax ? Row.Max : Stream.FRandRange(Row.Min, Row.Max);
				Result.Magnitudes.Add(Magnitude);
			});

		return Result;
	}
}

TArray<FRolledEnchantOption> EnchantOptionRoller::RollOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream)
{
	TArray<FRolledEnchantOption> Result;
	if (!IsValid(Def)) return Result;

	const int32 SlotCount = EnchantSlotPolicy::GetBonusSlotCount(GradeTag);
	if (SlotCount <= 0) return Result;

	const UItemFragment_Enchantable* Fragment = Def->FindFragment<UItemFragment_Enchantable>();
	if (Fragment == nullptr) return Result;

	UDataTable* Pool = Fragment->EnchantOptionPoolTable.LoadSynchronous();
	if (!IsValid(Pool)) return Result;

	const bool bEngraved = GradeTag.MatchesTagExact(GYGameplayTags::Item_Grade_Legendary_Engraved);

	TArray<FRollCandidate> Candidates = GatherCandidates(Pool);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		if (Candidates.IsEmpty()) break;
		const FName Picked = PickWeighted(Candidates, Stream);
		if (Picked.IsNone()) break;

		Result.Add(RollMagnitudes(Picked, bEngraved, Stream));
		Candidates.RemoveAll([&Picked](const FRollCandidate& Candidate)
		{
			return Candidate.Id == Picked;
		});
	}

	return Result;
}

TArray<FRolledEnchantOption> EnchantOptionRoller::RollAllOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream)
{
	TArray<FRolledEnchantOption> Result = RollOptions(Def, GradeTag, Stream);

	if (GradeTag.MatchesTagExact(GYGameplayTags::Item_Grade_Legendary_Engraved))
	{
		const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();
		UDataTable* PenaltyPool = IsValid(Settings) ? Settings->PenaltyOptionTable.LoadSynchronous() : nullptr;
		if (IsValid(PenaltyPool))
		{
			TArray<FRollCandidate> PenaltyCandidates = GatherCandidates(PenaltyPool);
			const FName PenaltyId = PickWeighted(PenaltyCandidates, Stream);
			if (!PenaltyId.IsNone())
			{
				// 페널티는 각인이어도 랜덤
				Result.Add(RollMagnitudes(PenaltyId, false, Stream));
			}
		}
	}

	return Result;
}
