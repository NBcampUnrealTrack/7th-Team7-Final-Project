#include "Equipment/EquipmentEntry.h"

#include "Equipment/EquipmentComponent.h"

void FEquipmentEntry::PreReplicatedRemove(const FEquipmentList& Serializer)
{
	if (::IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnEquipmentChanged.Broadcast(SlotTag, nullptr);
	}
}

void FEquipmentEntry::PostReplicatedAdd(const FEquipmentList& Serializer)
{
	if (::IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnEquipmentChanged.Broadcast(SlotTag, Instance);
	}
}

void FEquipmentEntry::PostReplicatedChange(const FEquipmentList& Serializer)
{
	if (::IsValid(Serializer.OwnerComponent))
	{
		Serializer.OwnerComponent->OnEquipmentChanged.Broadcast(SlotTag, Instance);
	}
}
