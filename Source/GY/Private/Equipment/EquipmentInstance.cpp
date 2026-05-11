#include "Equipment/EquipmentInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "Items/ItemDefinition.h"

void UEquipmentInstance::Initialize(const FGuid& InInstanceId, TSoftObjectPtr<UItemDefinition> InDefinition)
{
	InstanceId = InInstanceId;
	ItemDefinition = InDefinition;
}

UItemDefinition* UEquipmentInstance::GetItemDefinition() const
{
	return ItemDefinition.Get();
}

void UEquipmentInstance::OnEquipped(APawn* OwningPawn)
{
	OwnerPawn = OwningPawn;
}

void UEquipmentInstance::OnUnequipped(APawn* OwningPawn)
{
	OwnerPawn = nullptr;
}

UAbilitySystemComponent* UEquipmentInstance::FindAbilitySystemComponent() const
{
	APawn* Pawn = OwnerPawn.Get();
	if (!::IsValid(Pawn)) return nullptr;

	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
}
