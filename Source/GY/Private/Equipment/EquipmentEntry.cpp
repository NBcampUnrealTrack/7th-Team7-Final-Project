#include "Equipment/EquipmentEntry.h"

#include "Equipment/EquipmentComponent.h"
#include "Equipment/EquipmentInstance.h"
#include "GameFramework/Pawn.h"

void FEquipmentEntry::PreReplicatedRemove(const FEquipmentList& Serializer)
{
	if (!::IsValid(Serializer.OwnerComponent)) return;

	if (::IsValid(Instance))
	{
		APawn* Pawn = Cast<APawn>(Serializer.OwnerComponent->GetOwner());
		Instance->OnUnequipped(Pawn);
	}

	Serializer.OwnerComponent->OnEquipmentChanged.Broadcast(SlotTag, nullptr);
}

void FEquipmentEntry::PostReplicatedAdd(const FEquipmentList& Serializer)
{
	if (!::IsValid(Serializer.OwnerComponent)) return;

	if (::IsValid(Instance))
	{
		APawn* Pawn = Cast<APawn>(Serializer.OwnerComponent->GetOwner());
		Instance->OnEquipped(Pawn);
	}

	Serializer.OwnerComponent->OnEquipmentChanged.Broadcast(SlotTag, Instance);
}

void FEquipmentEntry::PostReplicatedChange(const FEquipmentList& Serializer)
{
	if (!::IsValid(Serializer.OwnerComponent)) return;

	Serializer.OwnerComponent->OnEquipmentChanged.Broadcast(SlotTag, Instance);
}
