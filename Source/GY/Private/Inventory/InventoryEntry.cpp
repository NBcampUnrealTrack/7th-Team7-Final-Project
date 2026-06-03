#include "Inventory/InventoryEntry.h"
#include "Items/ItemContainer.h"
#include "Inventory/InventoryComponent.h"

void FInventoryEntry::PreReplicatedRemove(const FInventoryList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent.GetObject()))
	{
		Serializer.OwnerComponent.GetInterface()->NotifyContainerChanged(InstanceId, EInventoryEventType::Removed);
	}
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent.GetObject()))
	{
		Serializer.OwnerComponent.GetInterface()->NotifyContainerChanged(InstanceId, EInventoryEventType::Added);
	}

}

void FInventoryEntry::PostReplicatedChange(const FInventoryList& Serializer)
{
	if (IsValid(Serializer.OwnerComponent.GetObject()))
	{
		Serializer.OwnerComponent.GetInterface()->NotifyContainerChanged(InstanceId, EInventoryEventType::Mutated);
	}

}
