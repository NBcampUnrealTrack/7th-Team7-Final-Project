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

void FInventoryList::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters)
{
	// 배열 델타가 모두 적용된 뒤라 일관 상태. InstanceId 없이 "전체 갱신" 신호만 보냄
	// (InstanceId를 보는 소비자는 위 per-item 콜백에서 이미 처리됨)
	if (IsValid(OwnerComponent.GetObject()))
	{
		OwnerComponent.GetInterface()->NotifyContainerChanged(FGuid(), EInventoryEventType::Mutated);
	}
}
