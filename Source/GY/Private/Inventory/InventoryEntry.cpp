#include "Inventory/InventoryEntry.h"

void FInventoryEntry::PreReplicatedRemove(const FInventoryList& Serializer)
{
	// TODO: OwnerComponent->OnInventoryChanged.Broadcast(InstanceId, EInventoryEventType::Removed) — C8 UInventoryComponent 추가 후
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryList& Serializer)
{
	// TODO: OwnerComponent->OnInventoryChanged.Broadcast(InstanceId, EInventoryEventType::Added) — C8 UInventoryComponent 추가 후
}

void FInventoryEntry::PostReplicatedChange(const FInventoryList& Serializer)
{
	// TODO: OwnerComponent->OnInventoryChanged.Broadcast(InstanceId, EInventoryEventType::Mutated) — C8 UInventoryComponent 추가 후
}
