#include "Character/GYCharacter.h"

#include "Equipment/EquipmentComponent.h"

AGYCharacter::AGYCharacter()
{
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
}
