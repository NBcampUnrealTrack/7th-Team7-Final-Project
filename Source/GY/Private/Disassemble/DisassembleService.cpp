#include "Disassemble/DisassembleService.h"

#include "Core/GameplayTags/CurrencyTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Currency/CurrencyComponent.h"
#include "Disassemble/DisassembleRewardRow.h"
#include "Disassemble/GYDisassembleSettings.h"
#include "Engine/DataTable.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Interaction/AltarStorageComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"

namespace
{
	bool LookupReward(UDataTable* Table, FGameplayTag GradeTag, int32 Level, FGameplayTag& OutCurrencyTag, int32& OutAmount)
	{
		if (!IsValid(Table)) return false;
		if (!GradeTag.IsValid()) return false;

		bool bFound = false;
		Table->ForeachRow<FDisassembleRewardRow>(TEXT("DisassembleService::LookupReward"),
			[&](const FName& RowName, const FDisassembleRewardRow& Row)
			{
				if (bFound) return;
				if (!Row.GradeTag.MatchesTagExact(GradeTag)) return;
				if (Row.Level != Level) return;

				OutCurrencyTag = Row.CurrencyTag.IsValid() ? Row.CurrencyTag : GYGameplayTags::Currency_TimeShard;
				OutAmount = Row.Amount;
				bFound = true;
			});
		return bFound;
	}

	bool IsInstanceEquipped(const UInventoryComponent* Inventory, const FGuid& InstanceId)
	{
		const AGYPlayerState* PS = Cast<AGYPlayerState>(Inventory->GetOwner());
		if (!IsValid(PS)) return false;

		const UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
		if (!IsValid(Loadout)) return false;

		for (const FEquipmentLoadoutEntry& Entry : Loadout->GetEntries())
		{
			if (Entry.InstanceId == InstanceId) return true;
		}
		return false;
	}
}

bool UDisassembleService::TryDisassemble(UAltarStorageComponent* Altar, UCurrencyComponent* Currency, const FGuid& InstanceId)
{
	if (!IsValid(Altar)) return false;
	if (!IsValid(Currency)) return false;
	if (!Altar->GetOwner()->HasAuthority()) return false;

	const FInventoryEntry* Entry = Altar->FindEntry(InstanceId);
	if (Entry == nullptr) return false;

	const UItemDefinition* Def = Entry->Definition.LoadSynchronous();
	if (!IsValid(Def)) return false;
	if (!Def->CategoryTags.HasTag(GYGameplayTags::Item_Category_Equipment))
	{
		UE_LOG(LogTemp, Warning, TEXT("Disassemble: %s is not equipment"), *Def->ItemId.ToString());
		return false;
	}

	const UGYDisassembleSettings* Settings = GetDefault<UGYDisassembleSettings>();
	UDataTable* RewardTable = IsValid(Settings) ? Settings->RewardTable.LoadSynchronous() : nullptr;

	FGameplayTag RewardCurrency;
	int32 RewardPerItem = 0;
	if (!LookupReward(RewardTable, Entry->GradeTag, Entry->Level, RewardCurrency, RewardPerItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("Disassemble: no reward configured for grade=%s level=%d"),
			*Entry->GradeTag.ToString(), Entry->Level);
		return false;
	}

	const int32 StackCount = Entry->StackCount;
	const int32 Reward = RewardPerItem * StackCount;

	if (!Altar->TryRemoveItem(InstanceId, StackCount)) return false;

	Currency->TryAdd(RewardCurrency, Reward);

	OnItemDisassembled.Broadcast(InstanceId, RewardCurrency, Reward);
	return true;
}
