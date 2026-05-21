#include "Enchant/EnchantOptionResolver.h"

#include "Enchant/GYEnchantSettings.h"
#include "Engine/DataTable.h"
#include "Items/EnchantOptionRow.h"
#include "Items/Fragments/ItemFragment_Enchantable.h"
#include "Items/ItemDefinition.h"

const FEnchantOptionRow* EnchantOptionResolver::FindRow(UItemDefinition* Def, FName OptionId)
{
	if (OptionId.IsNone()) return nullptr;

	if (IsValid(Def))
	{
		const UItemFragment_Enchantable* Fragment = Def->FindFragment<UItemFragment_Enchantable>();
		if (Fragment != nullptr)
		{
			UDataTable* ItemPool = Fragment->EnchantOptionPoolTable.LoadSynchronous();
			if (IsValid(ItemPool))
			{
				const FEnchantOptionRow* Row = ItemPool->FindRow<FEnchantOptionRow>(
					OptionId, TEXT("EnchantOptionResolver::FindRow"));
				if (Row != nullptr) return Row;
			}
		}
	}

	const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();
	UDataTable* PenaltyPool = IsValid(Settings) ? Settings->PenaltyOptionTable.LoadSynchronous() : nullptr;
	if (IsValid(PenaltyPool))
	{
		return PenaltyPool->FindRow<FEnchantOptionRow>(
			OptionId, TEXT("EnchantOptionResolver::FindRow.Penalty"));
	}

	return nullptr;
}
