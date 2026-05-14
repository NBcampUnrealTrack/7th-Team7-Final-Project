#include "Equipment/EquipmentLoadoutComponent.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/Fragments/ItemFragment_Equippable.h"
#include "Items/ItemDefinition.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"

UEquipmentLoadoutComponent::UEquipmentLoadoutComponent()
{
	SetIsReplicatedByDefault(true);
}

void UEquipmentLoadoutComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UEquipmentLoadoutComponent, LoadoutEntries, Params);
}

void UEquipmentLoadoutComponent::Server_RequestEquip_Implementation(const FGuid& InstanceId)
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwner());
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	const FInventoryEntry* Entry = Inv->FindEntry(InstanceId);
	if (Entry == nullptr) return;

	UItemDefinition* Def = Entry->Definition.LoadSynchronous();
	if (!IsValid(Def)) return;

	const UItemFragment_Equippable* EquippableFragment = Def->FindFragment<UItemFragment_Equippable>();
	if (EquippableFragment == nullptr) return;

	SetSlot(EquippableFragment->SlotTag, InstanceId);
}

void UEquipmentLoadoutComponent::Server_RequestUnequip_Implementation(FGameplayTag SlotTag)
{
	ClearSlot(SlotTag);
}

bool UEquipmentLoadoutComponent::SetSlot(FGameplayTag SlotTag, const FGuid& InstanceId)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!SlotTag.IsValid()) return false;
	if (!InstanceId.IsValid()) return false;

	const int32 Index = LoadoutEntries.IndexOfByPredicate([&SlotTag](const FEquipmentLoadoutEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	if (Index == INDEX_NONE)
	{
		FEquipmentLoadoutEntry NewEntry;
		NewEntry.SlotTag = SlotTag;
		NewEntry.InstanceId = InstanceId;
		LoadoutEntries.Add(NewEntry);
	}
	else
	{
		if (LoadoutEntries[Index].InstanceId == InstanceId) return false;
		LoadoutEntries[Index].InstanceId = InstanceId;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UEquipmentLoadoutComponent, LoadoutEntries, this);
	OnLoadoutSlotChanged.Broadcast(SlotTag, InstanceId);

	return true;
}

bool UEquipmentLoadoutComponent::ClearSlot(FGameplayTag SlotTag)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!SlotTag.IsValid()) return false;

	const int32 Index = LoadoutEntries.IndexOfByPredicate([&SlotTag](const FEquipmentLoadoutEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	if (Index == INDEX_NONE) return false;

	LoadoutEntries.RemoveAt(Index);

	MARK_PROPERTY_DIRTY_FROM_NAME(UEquipmentLoadoutComponent, LoadoutEntries, this);
	OnLoadoutSlotChanged.Broadcast(SlotTag, FGuid());

	return true;
}

bool UEquipmentLoadoutComponent::GetSlot(FGameplayTag SlotTag, FGuid& OutInstanceId) const
{
	const FEquipmentLoadoutEntry* Found = LoadoutEntries.FindByPredicate([&SlotTag](const FEquipmentLoadoutEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	if (Found == nullptr) return false;

	OutInstanceId = Found->InstanceId;
	return true;
}

void UEquipmentLoadoutComponent::OnRep_LoadoutEntries(const TArray<FEquipmentLoadoutEntry>& OldEntries)
{
	for (const FEquipmentLoadoutEntry& OldEntry : OldEntries)
	{
		const FEquipmentLoadoutEntry* NewEntry = LoadoutEntries.FindByPredicate([&OldEntry](const FEquipmentLoadoutEntry& Entry)
		{
			return Entry.SlotTag == OldEntry.SlotTag;
		});

		if (NewEntry == nullptr)
		{
			OnLoadoutSlotChanged.Broadcast(OldEntry.SlotTag, FGuid());
		}
		else if (NewEntry->InstanceId != OldEntry.InstanceId)
		{
			OnLoadoutSlotChanged.Broadcast(NewEntry->SlotTag, NewEntry->InstanceId);
		}
	}

	for (const FEquipmentLoadoutEntry& NewEntry : LoadoutEntries)
	{
		const bool bWasInOld = OldEntries.ContainsByPredicate([&NewEntry](const FEquipmentLoadoutEntry& Entry)
		{
			return Entry.SlotTag == NewEntry.SlotTag;
		});

		if (!bWasInOld)
		{
			OnLoadoutSlotChanged.Broadcast(NewEntry.SlotTag, NewEntry.InstanceId);
		}
	}
}
