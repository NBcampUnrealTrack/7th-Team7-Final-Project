#include "Player/GYPlayerState.h"

#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerDamageAttributeSet.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Character/GYPawnData.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Currency/CurrencyComponent.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Interaction/AltarStorageComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTransactionComponent.h"
#include "Loot/LootViewerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerInitData.h"
#include "SkillTree/SkillTreeComponent.h"

AGYPlayerState::AGYPlayerState()
{
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UGYAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	VitalAttribute = CreateDefaultSubobject<UGYPlayerVitalAttributeSet>(TEXT("VitalAttribute"));
	DamageAttribute = CreateDefaultSubobject<UGYPlayerDamageAttributeSet>(TEXT("DamageAttribute"));
	PlayerAttribute = CreateDefaultSubobject<UGYPlayerAttribute>(TEXT("PlayerAttribute"));
	WeaponAttribute = CreateDefaultSubobject<UGYWeaponAttribute>(TEXT("WeaponAttribute"));

	AbilitySystemComponent->AddAttributeSetSubobject(VitalAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(DamageAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(PlayerAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(WeaponAttribute.Get());

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	AltarStorageComponent = CreateDefaultSubobject<UAltarStorageComponent>(TEXT("AltarStorageComponent"));
	EquipmentLoadoutComponent = CreateDefaultSubobject<UEquipmentLoadoutComponent>(TEXT("EquipmentLoadoutComponent"));
	CurrencyComponent = CreateDefaultSubobject<UCurrencyComponent>(TEXT("CurrencyComponent"));
	SkillTreeComponent = CreateDefaultSubobject<USkillTreeComponent>(TEXT("SkillTreeComponent"));
	LootViewerComponent = CreateDefaultSubobject<ULootViewerComponent>(TEXT("LootViewerComponent"));
	ItemTransactionComponent = CreateDefaultSubobject<UItemTransactionComponent>(TEXT("ItemTransactionComponent"));
}

UAbilitySystemComponent* AGYPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGYPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYPlayerState, PawnData);
	DOREPLIFETIME(AGYPlayerState, LastCheckpointLocation);
	DOREPLIFETIME(AGYPlayerState, bHasCheckpoint);
}

void AGYPlayerState::SetPawnData(const UGYPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority) return;
	if (PawnData) return;

	PawnData = InPawnData;
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
	if (bAttributesInitialized) return;
	bAttributesInitialized = true;
	InitialSpawnLocation = Avatar->GetActorLocation();

	AbilitySystemComponent->AddLooseGameplayTag(
		GYFactionTags::Character_Faction_Player, 1,
		EGameplayTagReplicationState::TagOnly);

	if (InitData)
	{
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetMaxHealthAttribute(),     InitData->MaxHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentHealthAttribute(), InitData->MaxHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UGYDamageAttributeSet::GetAttackAttribute(),        InitData->Attack);
		AbilitySystemComponent->SetNumericAttributeBase(UGYDamageAttributeSet::GetDefenseAttribute(),       InitData->Defense);

		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerAttribute::GetMaxStaminaAttribute(),     InitData->MaxStamina);
		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerAttribute::GetCurrentStaminaAttribute(), InitData->MaxStamina);
		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerAttribute::GetStrengthAttribute(),       InitData->Strength);
		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerAttribute::GetDexterityAttribute(),      InitData->Dexterity);

		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetMaxStaggerAttribute(),     InitData->MaxStagger);
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentStaggerAttribute(), InitData->MaxStagger);
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetMaxStunAttribute(),        InitData->MaxStun);
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentStunAttribute(),    InitData->MaxStun);
	}
}

void AGYPlayerState::Server_SetCheckpoint_Implementation(FVector Location)
{
	LastCheckpointLocation = Location;
	bHasCheckpoint = true;
}

