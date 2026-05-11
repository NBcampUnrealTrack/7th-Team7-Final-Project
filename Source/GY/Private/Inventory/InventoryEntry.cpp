#include "Inventory/InventoryEntry.h"

#include "Inventory/InventoryComponent.h"

void FInventoryEntry::PreReplicatedRemove(const FInventoryList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnInventoryChanged.Broadcast(InstanceId, EInventoryEventType::Removed);
	}
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnInventoryChanged.Broadcast(InstanceId, EInventoryEventType::Added);
	}
}

void FInventoryEntry::PostReplicatedChange(const FInventoryList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnInventoryChanged.Broadcast(InstanceId, EInventoryEventType::Mutated);
	}
}
