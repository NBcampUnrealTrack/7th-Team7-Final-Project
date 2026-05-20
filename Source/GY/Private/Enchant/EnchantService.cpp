#include "Enchant/EnchantService.h"

#include "Core/GameplayTags/ItemTags.h"
#include "Currency/CurrencyComponent.h"
#include "Enchant/EnchantCostRow.h"
#include "Enchant/EnchantSlotPolicy.h"
#include "Enchant/GYEnchantSettings.h"
#include "Engine/DataTable.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
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

		Pool->ForeachRow<FEnchantOptionRow>(TEXT("EnchantService::Roll"),
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

	FName RollPenaltyOption(const UGYEnchantSettings* Settings, FRandomStream& Stream)
	{
		if (!IsValid(Settings)) return NAME_None;

		UDataTable* PenaltyPool = Settings->PenaltyOptionTable.LoadSynchronous();
		if (!IsValid(PenaltyPool)) return NAME_None;

		const TArray<FRollCandidate> Candidates = GatherCandidates(PenaltyPool);
		return PickWeighted(Candidates, Stream);
	}
}

bool UEnchantService::TryEnchant(UInventoryComponent* Inventory,
	UCurrencyComponent* Currency,
	const FGuid& InstanceId,
	const FRandomStream& Seed,
	TArray<FName>& OutRolledIds)
{
	OutRolledIds.Reset();

	if (!IsValid(Inventory)) return false;
	if (!IsValid(Currency)) return false;
	if (!Inventory->GetOwner()->HasAuthority()) return false;

	const FInventoryEntry* Entry = Inventory->FindEntry(InstanceId);
	if (Entry == nullptr) return false;

	UItemDefinition* Def = Entry->Definition.LoadSynchronous();
	if (!IsValid(Def)) return false;

	const UItemFragment_Enchantable* Fragment = Def->FindFragment<UItemFragment_Enchantable>();
	if (Fragment == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enchant: %s has no Enchantable fragment"), *Def->ItemId.ToString());
		return false;
	}

	UDataTable* Pool = Fragment->EnchantOptionPoolTable.LoadSynchronous();
	if (!IsValid(Pool))
	{
		UE_LOG(LogTemp, Warning, TEXT("Enchant: %s has null EnchantOptionPoolTable"), *Def->ItemId.ToString());
		return false;
	}

	TArray<FRollCandidate> Candidates = GatherCandidates(Pool);
	if (Candidates.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Enchant: pool table for %s has no valid rows"), *Def->ItemId.ToString());
		return false;
	}

	const int32 SlotCount = EnchantSlotPolicy::GetBonusSlotCount(Entry->GradeTag);
	if (SlotCount <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Enchant: grade %s has no bonus slots"), *Entry->GradeTag.ToString());
		return false;
	}

	const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();
	UDataTable* CostTable = IsValid(Settings) ? Settings->EnchantCostTable.LoadSynchronous() : nullptr;
	if (IsValid(CostTable))
	{
		const FEnchantCostRow* CostRow = CostTable->FindRow<FEnchantCostRow>(
			Settings->DefaultCostRowName, TEXT("EnchantService::TryEnchant"));
		if (CostRow != nullptr && CostRow->Amount > 0)
		{
			if (!Currency->TrySpend(CostRow->CurrencyTag, CostRow->Amount))
			{
				UE_LOG(LogTemp, Log, TEXT("Enchant: insufficient %s (need %d)"),
					*CostRow->CurrencyTag.ToString(), CostRow->Amount);
				return false;
			}
		}
	}

	FRandomStream Stream = Seed;
	TArray<FName> Rolled;
	for (int32 i = 0; i < SlotCount; ++i)
	{
		if (Candidates.IsEmpty()) break;

		const FName Picked = PickWeighted(Candidates, Stream);
		if (Picked.IsNone()) break;

		Rolled.Add(Picked);
		Candidates.RemoveAll([&Picked](const FRollCandidate& Candidate)
		{
			return Candidate.Id == Picked;
		});
	}

	if (Rolled.IsEmpty()) return false;

	if (Entry->GradeTag.MatchesTagExact(GYGameplayTags::Item_Grade_Legendary_Engraved))
	{
		const FName PenaltyId = RollPenaltyOption(Settings, Stream);
		if (!PenaltyId.IsNone()) Rolled.Add(PenaltyId);
	}

	Inventory->MutateEntry(InstanceId, [&Rolled](FInventoryEntry& E)
	{
		E.EnchantOptionIds = Rolled;
	});

	OutRolledIds = Rolled;
	OnItemEnchanted.Broadcast(InstanceId);
	return true;
}
