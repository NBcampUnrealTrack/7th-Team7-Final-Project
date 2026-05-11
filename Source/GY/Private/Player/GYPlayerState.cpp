#include "Player/GYPlayerState.h"

#include "Inventory/InventoryComponent.h"

AGYPlayerState::AGYPlayerState()
{
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}
