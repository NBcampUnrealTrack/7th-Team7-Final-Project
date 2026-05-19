#include "Player/GYPlayerState.h"

#include "AbilitySystem/Attributes/CombatAttributeSet.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerBaseAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/InventoryComponent.h"

AGYPlayerState::AGYPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGYAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributeSet"));

	BaseAttribute = CreateDefaultSubobject<UGYPlayerBaseAttribute>(TEXT("BaseAttribute"));
	AdditionalAttribute = CreateDefaultSubobject<UGYPlayerAdditionalAttribute>(TEXT("AdditionalAttribute"));
	PlayerAttribute = CreateDefaultSubobject<UGYPlayerAttribute>(TEXT("PlayerAttribute"));

	AbilitySystemComponent->GetSpawnedAttributes_Mutable().AddUnique(BaseAttribute);
	AbilitySystemComponent->GetSpawnedAttributes_Mutable().AddUnique(AdditionalAttribute);
	AbilitySystemComponent->GetSpawnedAttributes_Mutable().AddUnique(PlayerAttribute);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentLoadoutComponent = CreateDefaultSubobject<UEquipmentLoadoutComponent>(TEXT("EquipmentLoadoutComponent"));
}

UAbilitySystemComponent* AGYPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
