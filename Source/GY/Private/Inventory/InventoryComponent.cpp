#include "Inventory/InventoryComponent.h"

#include "Items/ItemDefinition.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	Inventory.OwnerComponent = this;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryComponent, Inventory, Params);
}

bool UInventoryComponent::TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (Count <= 0) return false;
	if (Def.IsNull()) return false;

	FInventoryEntry NewEntry;
	NewEntry.InstanceId = FGuid::NewGuid();
	NewEntry.Definition = Def;
	NewEntry.StackCount = Count;

	FInventoryEntry& AddedEntry = Inventory.Entries.Add_GetRef(NewEntry);
	Inventory.MarkItemDirty(AddedEntry);

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	OutInstanceId = AddedEntry.InstanceId;

	OnInventoryChanged.Broadcast(AddedEntry.InstanceId, EInventoryEventType::Added);

	return true;
}

bool UInventoryComponent::TryRemoveItem(const FGuid& InstanceId, int32 Count)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (Count <= 0) return false;

	const int32 Index = Inventory.Entries.IndexOfByPredicate([&InstanceId](const FInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});

	if (Index == INDEX_NONE) return false;

	FInventoryEntry& Entry = Inventory.Entries[Index];
	Entry.StackCount -= Count;

	EInventoryEventType EventType = EInventoryEventType::StackCountChanged;

	if (Entry.StackCount <= 0)
	{
		Inventory.Entries.RemoveAt(Index);
		Inventory.MarkArrayDirty();
		EventType = EInventoryEventType::Removed;
	}
	else
	{
		Inventory.MarkItemDirty(Entry);
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	OnInventoryChanged.Broadcast(InstanceId, EventType);

	return true;
}

bool UInventoryComponent::MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator)
{
	if (!GetOwner()->HasAuthority()) return false;

	FInventoryEntry* Entry = Inventory.Entries.FindByPredicate([&InstanceId](const FInventoryEntry& E)
	{
		return E.InstanceId == InstanceId;
	});

	if (Entry == nullptr) return false;

	Mutator(*Entry);
	Inventory.MarkItemDirty(*Entry);

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	OnInventoryChanged.Broadcast(InstanceId, EInventoryEventType::Mutated);

	return true;
}

const FInventoryEntry* UInventoryComponent::FindEntry(const FGuid& InstanceId) const
{
	return Inventory.Entries.FindByPredicate([&InstanceId](const FInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});
}

TArray<FInventoryEntry> UInventoryComponent::GetAllEntriesByCategory(FGameplayTag CategoryTag) const
{
	TArray<FInventoryEntry> Result;

	for (const FInventoryEntry& Entry : Inventory.Entries)
	{
		const UItemDefinition* Def = Entry.Definition.LoadSynchronous();
		if (IsValid(Def) && Def->CategoryTags.HasTag(CategoryTag))
		{
			Result.Add(Entry);
		}
	}

	return Result;
}
