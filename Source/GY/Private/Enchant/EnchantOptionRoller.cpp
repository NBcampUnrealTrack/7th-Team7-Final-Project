#include "Enchant/EnchantOptionRoller.h"

#include "Core/GameplayTags/ItemTags.h"
#include "Enchant/EnchantSlotPolicy.h"
#include "Enchant/GYEnchantSettings.h"
#include "Engine/DataTable.h"
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
}

TArray<FName> EnchantOptionRoller::RollOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream)
{
	TArray<FName> Result;
	if (!IsValid(Def)) return Result;

	const int32 SlotCount = EnchantSlotPolicy::GetBonusSlotCount(GradeTag);
	if (SlotCount <= 0) return Result;

	const UItemFragment_Enchantable* Fragment = Def->FindFragment<UItemFragment_Enchantable>();
	if (Fragment == nullptr) return Result;

	UDataTable* Pool = Fragment->EnchantOptionPoolTable.LoadSynchronous();
	if (!IsValid(Pool)) return Result;

	TArray<FRollCandidate> Candidates = GatherCandidates(Pool);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		if (Candidates.IsEmpty()) break;
		const FName Picked = PickWeighted(Candidates, Stream);
		if (Picked.IsNone()) break;
		Result.Add(Picked);
		Candidates.RemoveAll([&Picked](const FRollCandidate& Candidate)
		{
			return Candidate.Id == Picked;
		});
	}

	return Result;
}

FName EnchantOptionRoller::RollPenalty(FRandomStream& Stream)
{
	const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();
	if (!IsValid(Settings)) return NAME_None;

	UDataTable* PenaltyPool = Settings->PenaltyOptionTable.LoadSynchronous();
	if (!IsValid(PenaltyPool)) return NAME_None;

	const TArray<FRollCandidate> Candidates = GatherCandidates(PenaltyPool);
	return PickWeighted(Candidates, Stream);
}

TArray<FName> EnchantOptionRoller::RollAllOptions(UItemDefinition* Def, FGameplayTag GradeTag, FRandomStream& Stream)
{
	TArray<FName> Result = RollOptions(Def, GradeTag, Stream);

	if (GradeTag.MatchesTagExact(GYGameplayTags::Item_Grade_Legendary_Engraved))
	{
		const FName PenaltyId = RollPenalty(Stream);
		if (!PenaltyId.IsNone()) Result.Add(PenaltyId);
	}

	return Result;
}
