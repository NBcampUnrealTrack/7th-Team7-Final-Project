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

	// 언젠가 BaseAttribute로 아이템 스탯 전환되면 옮겨주셍요
	CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributeSet"));

	BaseAttribute = CreateDefaultSubobject<UGYPlayerBaseAttribute>(TEXT("BaseAttribute"));
	AdditionalAttribute = CreateDefaultSubobject<UGYPlayerAdditionalAttribute>(TEXT("AdditionalAttribute"));
	PlayerAttribute = CreateDefaultSubobject<UGYPlayerAttribute>(TEXT("PlayerAttribute"));
	WeaponAttribute = CreateDefaultSubobject<UGYWeaponAttribute>(TEXT("WeaponAttribute"));


	AbilitySystemComponent->GetSpawnedAttributes_Mutable().AddUnique(BaseAttribute);
	AbilitySystemComponent->GetSpawnedAttributes_Mutable().AddUnique(AdditionalAttribute);
	AbilitySystemComponent->GetSpawnedAttributes_Mutable().AddUnique(PlayerAttribute);
	AbilitySystemComponent->GetSpawnedAttributes_Mutable().AddUnique(WeaponAttribute);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentLoadoutComponent = CreateDefaultSubobject<UEquipmentLoadoutComponent>(TEXT("EquipmentLoadoutComponent"));
}

UAbilitySystemComponent* AGYPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
