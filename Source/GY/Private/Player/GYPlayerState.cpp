#include "Player/GYPlayerState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AbilitySet.h"
#include "AbilitySystem/Attributes/CombatAttributeSet.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerBaseAttribute.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Character/GYPawnData.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
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

void AGYPlayerState::InitTestGAS(APawn* Avatar)
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

	if (CombatAbilitySet)
	{
		CombatAbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles);
	}
}

void AGYPlayerState::HandleAttackInput()
{
	if (!AbilitySystemComponent) return;

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability &&
			Spec.Ability->AbilityTags.HasTag(GYGameplayTags::Ability_Attack_Charge))
		{
			return;
		}
	}

	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(GYGameplayTags::Ability_Attack_Combo));

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Input_Attack;
	Payload.Instigator = GetPawn();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), GYGameplayTags::Event_Input_Attack, Payload);

	if (GetWorld() && HoldToChargeTime > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			HoldToChargeTimer,
			this,
			&AGYPlayerState::OnHoldToChargeThreshold,
			HoldToChargeTime,
			false
		);
	}
}

void AGYPlayerState::HandleAttackReleasedInput()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoldToChargeTimer);
	}

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Input_AttackRelease;
	Payload.Instigator = GetPawn();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), GYGameplayTags::Event_Input_AttackRelease, Payload);
}

void AGYPlayerState::HandleParryInput()
{
	if (!AbilitySystemComponent) return;

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Input_Parry;
	Payload.Instigator = GetPawn();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), GYGameplayTags::Event_Input_Parry, Payload);

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability &&
			(Spec.Ability->AbilityTags.HasTag(GYGameplayTags::Ability_Attack_Combo) ||
			 Spec.Ability->AbilityTags.HasTag(GYGameplayTags::Ability_Attack_Charge)))
		{
			return;
		}
	}

	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(GYGameplayTags::Ability_Parry));
}

void AGYPlayerState::OnHoldToChargeThreshold()
{
	if (!AbilitySystemComponent) return;

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Input_AttackCharge;
	Payload.Instigator = GetPawn();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), GYGameplayTags::Event_Input_AttackCharge, Payload);

	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(GYGameplayTags::Ability_Attack_Charge));
}
