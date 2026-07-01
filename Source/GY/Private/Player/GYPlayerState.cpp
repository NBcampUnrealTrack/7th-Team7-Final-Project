#include "Player/GYPlayerState.h"

#include "Character/GYPawnExtensionComponent.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerDamageAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Character/GYPawnData.h"
#include "Currency/CurrencyComponent.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Interaction/AltarStorageComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTransactionComponent.h"
#include "Loot/LootViewerComponent.h"
#include "Persistence/StatPersistenceComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerBaseStatsRow.h"
#include "SkillTree/SkillTreeComponent.h"

AGYPlayerState::AGYPlayerState()
{
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UGYAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	VitalAttribute = CreateDefaultSubobject<UGYPlayerVitalAttributeSet>(TEXT("VitalAttribute"));
	DamageAttribute = CreateDefaultSubobject<UGYPlayerDamageAttributeSet>(TEXT("DamageAttribute"));
	CoreStatAttribute = CreateDefaultSubobject<UGYCoreStatAttributeSet>(TEXT("CoreStatAttribute"));
	ProgressionAttribute = CreateDefaultSubobject<UGYProgressionAttributeSet>(TEXT("ProgressionAttribute"));

	AbilitySystemComponent->AddAttributeSetSubobject(VitalAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(DamageAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(CoreStatAttribute.Get());
	AbilitySystemComponent->AddAttributeSetSubobject(ProgressionAttribute.Get());

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	AltarStorageComponent = CreateDefaultSubobject<UAltarStorageComponent>(TEXT("AltarStorageComponent"));
	EquipmentLoadoutComponent = CreateDefaultSubobject<UEquipmentLoadoutComponent>(TEXT("EquipmentLoadoutComponent"));
	CurrencyComponent = CreateDefaultSubobject<UCurrencyComponent>(TEXT("CurrencyComponent"));
	SkillTreeComponent = CreateDefaultSubobject<USkillTreeComponent>(TEXT("SkillTreeComponent"));
	LootViewerComponent = CreateDefaultSubobject<ULootViewerComponent>(TEXT("LootViewerComponent"));
	ItemTransactionComponent = CreateDefaultSubobject<UItemTransactionComponent>(TEXT("ItemTransactionComponent"));
	StatPersistenceComponent = CreateDefaultSubobject<UStatPersistenceComponent>(TEXT("StatPersistenceComponent"));
}

UAbilitySystemComponent* AGYPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGYPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYPlayerState, PawnData);
	DOREPLIFETIME(AGYPlayerState, LastCheckpointId);
}

void AGYPlayerState::SetPawnData(const UGYPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority) return;
	if (PawnData) return;

	PawnData = InPawnData;
	ForceNetUpdate();
}

void AGYPlayerState::SetLastCheckpointId(const FGuid& Id)
{
	if (!HasAuthority()) return;
	LastCheckpointId = Id;
	ForceNetUpdate();
}

void AGYPlayerState::OnRep_PawnData()
{
	// PS.PawnData가 폰보다 늦게 복제돼 와도 초기화가 마저 진행되도록 다시 검사시킨다.
	UGYPawnExtensionComponent::RequestInitStateRecheck(GetPawn());
}

void AGYPlayerState::OnRep_Owner()
{
	Super::OnRep_Owner();

	// Owner(소유 컨트롤러)가 폰의 초기화 검사보다 늦게 복제돼 와도 초기화가 마저 진행되도록 다시 검사시킨다.
	UGYPawnExtensionComponent::RequestInitStateRecheck(GetPawn());
}

void AGYPlayerState::InitializeBaseAttributes()
{
	if (!AbilitySystemComponent) return;
	if (!HasAuthority()) return;

	const UGYPawnData* CurrentPawnData = GetPawnData();
	if (!CurrentPawnData) return;

	UDataTable* StatsTable = CurrentPawnData->BaseStatsTable.LoadSynchronous();
	const FGYPlayerBaseStatsRow* Stats = StatsTable
		? StatsTable->FindRow<FGYPlayerBaseStatsRow>(CurrentPawnData->BaseStatsRowName, TEXT("InitializeBaseAttributes"))
		: nullptr;
	if (Stats)
	{
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetMaxHealthAttribute(),     Stats->MaxHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentHealthAttribute(), Stats->MaxHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UGYDamageAttributeSet::GetAttackAttribute(),        Stats->Attack);
		AbilitySystemComponent->SetNumericAttributeBase(UGYDamageAttributeSet::GetDefenseAttribute(),       Stats->Defense);

		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetMaxStaminaAttribute(),     Stats->MaxStamina);
		AbilitySystemComponent->SetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute(), Stats->MaxStamina);
		AbilitySystemComponent->SetNumericAttributeBase(UGYCoreStatAttributeSet::GetStrengthAttribute(),       Stats->Strength);
		AbilitySystemComponent->SetNumericAttributeBase(UGYCoreStatAttributeSet::GetDexterityAttribute(),      Stats->Dexterity);

		// 경직/무력화는 누적 통: 0에서 시작해 피격으로 차오르고, Max 도달 시 발동.
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetMaxStaggerAttribute(),     Stats->MaxStagger);
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentStaggerAttribute(), 0.f);
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetMaxStunAttribute(),        Stats->MaxStun);
		AbilitySystemComponent->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentStunAttribute(),    0.f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeBaseAttributes: BaseStats 행을 찾지 못함 (Table=%s, Row=%s)"),
			*GetNameSafe(StatsTable), *CurrentPawnData->BaseStatsRowName.ToString());
	}

	// 파생 스탯(STR/DEX 기반) 무한 GE 적용. 1차 스탯 base 세팅 이후에 적용해야 캡처값이 맞음.
	if (CurrentPawnData->DerivedStatsEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(CurrentPawnData->DerivedStatsEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

