#include "Player/GYPlayerState.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Combat/CombatAttributeSet.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/InventoryComponent.h"

AGYPlayerState::AGYPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGYAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributeSet"));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentLoadoutComponent = CreateDefaultSubobject<UEquipmentLoadoutComponent>(TEXT("EquipmentLoadoutComponent"));
}

UAbilitySystemComponent* AGYPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
