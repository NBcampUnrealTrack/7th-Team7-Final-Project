#include "Character/GYCharacter.h"

#include "AbilitySystemComponent.h"
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

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
	{
		ASC->InitAbilityActorInfo(PS, this);
	}

	if (!IsValid(ActiveEquipmentComponent)) return;

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

void AGYCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
	{
		ASC->InitAbilityActorInfo(PS, this);
	}
}

UAbilitySystemComponent* AGYCharacter::GetAbilitySystemComponent() const
{
	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		return PS->GetAbilitySystemComponent();
	}
	return nullptr;
}
