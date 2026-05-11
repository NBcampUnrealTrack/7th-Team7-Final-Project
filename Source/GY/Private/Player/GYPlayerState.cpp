#include "Player/GYPlayerState.h"

#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/InventoryComponent.h"

AGYPlayerState::AGYPlayerState()
{
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentLoadoutComponent = CreateDefaultSubobject<UEquipmentLoadoutComponent>(TEXT("EquipmentLoadoutComponent"));
}
