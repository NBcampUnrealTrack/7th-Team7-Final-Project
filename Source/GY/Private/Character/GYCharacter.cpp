#include "Character/GYCharacter.h"

#include "Equipment/ActiveEquipmentComponent.h"

AGYCharacter::AGYCharacter()
{
	ActiveEquipmentComponent = CreateDefaultSubobject<UActiveEquipmentComponent>(TEXT("ActiveEquipmentComponent"));
}
