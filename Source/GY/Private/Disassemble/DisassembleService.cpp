#include "Disassemble/DisassembleService.h"

#include "Core/GameplayTags/CurrencyTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Currency/CurrencyComponent.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"

namespace
{
	constexpr int32 Reward_Normal = 1;
	constexpr int32 Reward_Special = 5;
	constexpr int32 Reward_Legendary = 20;
	constexpr int32 Reward_Unspecified = 1;

	int32 ComputeReward(FGameplayTag GradeTag)
	{
		if (GradeTag == GYGameplayTags::Item_Grade_Legendary) return Reward_Legendary;
		if (GradeTag == GYGameplayTags::Item_Grade_Special) return Reward_Special;
		if (GradeTag == GYGameplayTags::Item_Grade_Normal) return Reward_Normal;
		return Reward_Unspecified;
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

bool UDisassembleService::TryDisassemble(UInventoryComponent* Inventory, UCurrencyComponent* Currency, const FGuid& InstanceId)
{
	if (!IsValid(Inventory)) return false;
	if (!IsValid(Currency)) return false;
	if (!Inventory->GetOwner()->HasAuthority()) return false;

	const FInventoryEntry* Entry = Inventory->FindEntry(InstanceId);
	if (Entry == nullptr) return false;

	const UItemDefinition* Def = Entry->Definition.LoadSynchronous();
	if (!IsValid(Def)) return false;
	if (!Def->CategoryTags.HasTag(GYGameplayTags::Item_Category_Equipment))
	{
		UE_LOG(LogTemp, Warning, TEXT("Disassemble: %s is not equipment"), *Def->ItemId.ToString());
		return false;
	}

	if (IsInstanceEquipped(Inventory, InstanceId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Disassemble: cannot disassemble equipped instance %s"), *InstanceId.ToString());
		return false;
	}

	const FGameplayTag GradeTag = Entry->GradeTag;
	const int32 StackCount = Entry->StackCount;
	const int32 Reward = ComputeReward(GradeTag) * StackCount;

	if (!Inventory->TryRemoveItem(InstanceId, StackCount)) return false;

	const FGameplayTag CurrencyTag = GYGameplayTags::Currency_TimeShard;
	Currency->TryAdd(CurrencyTag, Reward);

	OnItemDisassembled.Broadcast(InstanceId, CurrencyTag, Reward);
	return true;
}
