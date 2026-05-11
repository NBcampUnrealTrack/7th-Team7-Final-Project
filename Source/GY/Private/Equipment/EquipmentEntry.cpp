#include "Equipment/EquipmentEntry.h"

#include "Equipment/ActiveEquipmentComponent.h"
#include "Equipment/EquipmentInstance.h"
#include "GameFramework/Pawn.h"

void FEquipmentEntry::PreReplicatedRemove(const FEquipmentList& Serializer)
{
	if (!IsValid(Serializer.OwnerComponent)) return;
	if (!IsValid(Instance)) return;

	APawn* Pawn = Cast<APawn>(Serializer.OwnerComponent->GetOwner());
	Instance->OnUnequipped(Pawn);
}

void FEquipmentEntry::PostReplicatedAdd(const FEquipmentList& Serializer)
{
	if (!IsValid(Serializer.OwnerComponent)) return;
	if (!IsValid(Instance)) return;

	APawn* Pawn = Cast<APawn>(Serializer.OwnerComponent->GetOwner());
	Instance->OnEquipped(Pawn);
}

void FEquipmentEntry::PostReplicatedChange(const FEquipmentList& Serializer)
{
}
