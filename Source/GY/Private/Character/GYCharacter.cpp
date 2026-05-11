#include "Character/GYCharacter.h"

#include "Equipment/ActiveEquipmentComponent.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Player/GYPlayerState.h"

AGYCharacter::AGYCharacter()
{
	ActiveEquipmentComponent = CreateDefaultSubobject<UActiveEquipmentComponent>(TEXT("ActiveEquipmentComponent"));
}

void AGYCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!HasAuthority()) return;
	if (!IsValid(ActiveEquipmentComponent)) return;

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Loadout)) return;

	Loadout->OnLoadoutSlotChanged.AddUObject(
		ActiveEquipmentComponent.Get(),
		&UActiveEquipmentComponent::OnLoadoutSlotChanged);

	for (const FEquipmentLoadoutEntry& Entry : Loadout->GetEntries())
	{
		ActiveEquipmentComponent->OnLoadoutSlotChanged(Entry.SlotTag, Entry.InstanceId);
	}
}
