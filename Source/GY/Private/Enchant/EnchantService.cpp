#include "Enchant/EnchantService.h"

#include "Currency/CurrencyComponent.h"
#include "Enchant/EnchantCostRow.h"
#include "Enchant/EnchantOptionRoller.h"
#include "Enchant/EnchantSlotPolicy.h"
#include "Enchant/GYEnchantSettings.h"
#include "Engine/DataTable.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/Fragments/ItemFragment_Enchantable.h"
#include "Items/ItemDefinition.h"

bool UEnchantService::TryEnchant(UInventoryComponent* Inventory,
	UCurrencyComponent* Currency,
	const FGuid& InstanceId,
	const FRandomStream& Seed,
	TArray<FRolledEnchantOption>& OutRolledOptions)
{
	OutRolledOptions.Reset();

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
	TArray<FRolledEnchantOption> Rolled = EnchantOptionRoller::RollAllOptions(Def, Entry->GradeTag, Stream);

	if (Rolled.IsEmpty()) return false;

	Inventory->MutateEntry(InstanceId, [&Rolled](FInventoryEntry& E)
	{
		E.RolledOptions = Rolled;
	});

	OutRolledOptions = Rolled;
	OnItemEnchanted.Broadcast(InstanceId);
	return true;
}
