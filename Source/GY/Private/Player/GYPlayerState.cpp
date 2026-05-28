#include "Player/GYPlayerState.h"

#include "AbilitySystem/AbilitySet.h"
#include "AbilitySystem/Attributes/CombatAttributeSet.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerBaseAttribute.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Character/GYPawnData.h"
#include "Currency/CurrencyComponent.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerInitData.h"

AGYPlayerState::AGYPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGYAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributeSet"));

	BaseAttribute = CreateDefaultSubobject<UGYPlayerBaseAttribute>(TEXT("BaseAttribute"));
	AdditionalAttribute = CreateDefaultSubobject<UGYPlayerAdditionalAttribute>(TEXT("AdditionalAttribute"));
	PlayerAttribute = CreateDefaultSubobject<UGYPlayerAttribute>(TEXT("PlayerAttribute"));
	WeaponAttribute = CreateDefaultSubobject<UGYWeaponAttribute>(TEXT("WeaponAttribute"));

	AbilitySystemComponent->AddAttributeSetSubobject(BaseAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(AdditionalAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(PlayerAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(WeaponAttribute.Get());

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentLoadoutComponent = CreateDefaultSubobject<UEquipmentLoadoutComponent>(TEXT("EquipmentLoadoutComponent"));
	CurrencyComponent = CreateDefaultSubobject<UCurrencyComponent>(TEXT("CurrencyComponent"));
}

UAbilitySystemComponent* AGYPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGYPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYPlayerState, PawnData);
}

void AGYPlayerState::SetPawnData(const UGYPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority) return;
	if (PawnData) return;

	PawnData = InPawnData;

	if (AbilitySystemComponent)
	{
		for (const UAbilitySet* AbilitySet : PawnData->AbilitySets)
		{
			if (AbilitySet)
			{
				AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles);
			}
		}
	}

	ForceNetUpdate();
}

void AGYPlayerState::OnRep_PawnData()
{
}

void AGYPlayerState::InitGAS(APawn* Avatar)
{
	if (!AbilitySystemComponent || !Avatar) return;

	AbilitySystemComponent->InitAbilityActorInfo(this, Avatar);

	if (GetLocalRole() != ROLE_Authority) return;

	if (InitData)
	{
		AbilitySystemComponent->SetNumericAttributeBase(UGYBaseAttribute::GetMaxHealthAttribute(),     InitData->MaxHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UGYBaseAttribute::GetCurrentHealthAttribute(), InitData->MaxHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UGYBaseAttribute::GetAttackAttribute(),        InitData->Attack);
		AbilitySystemComponent->SetNumericAttributeBase(UGYBaseAttribute::GetDefenseAttribute(),       InitData->Defense);

		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerAttribute::GetMaxStaminaAttribute(),     InitData->MaxStamina);
		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerAttribute::GetCurrentStaminaAttribute(), InitData->MaxStamina);
		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerAttribute::GetStrengthAttribute(),       InitData->Strength);
		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerAttribute::GetDexterityAttribute(),      InitData->Dexterity);

		AbilitySystemComponent->SetNumericAttributeBase(UGYAdditionalAttribute::GetMaxStaggerAttribute(),     InitData->MaxStagger);
		AbilitySystemComponent->SetNumericAttributeBase(UGYAdditionalAttribute::GetCurrentStaggerAttribute(), InitData->MaxStagger);
		AbilitySystemComponent->SetNumericAttributeBase(UGYAdditionalAttribute::GetMaxStunAttribute(),        InitData->MaxStun);
		AbilitySystemComponent->SetNumericAttributeBase(UGYAdditionalAttribute::GetCurrentStunAttribute(),    InitData->MaxStun);
	}
}

