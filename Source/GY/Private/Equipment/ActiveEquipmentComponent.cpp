#include "Equipment/ActiveEquipmentComponent.h"

#include "AbilitySystem/AbilitySet.h"
#include "AbilitySystem/GYOnHitModifierComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/OptionTags.h"
#include "Enchant/EnchantMagnitudeEffectRow.h"
#include "Enchant/EnchantService.h"
#include "Enchant/GYEnchantSettings.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Equipment/EquipmentInstance.h"
#include "Equipment/GYEquipmentSettings.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/Fragments/ItemFragment_Equippable.h"
#include "Items/Fragments/ItemFragment_GrantedAbilitySet.h"
#include "Items/Fragments/ItemFragment_Weapon.h"
#include "Items/ItemDefinition.h"
#include "Items/WeaponBaseStatsRow.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"

static FGameplayTag GetWeaponTypeTag(const UItemDefinition* ItemDefinition)
{
	if (!IsValid(ItemDefinition))
	{
		return FGameplayTag();
	}

	const UItemFragment_Weapon* WeaponFragment = ItemDefinition->FindFragment<UItemFragment_Weapon>();
	if (WeaponFragment == nullptr)
	{
		return FGameplayTag();
	}

	return WeaponFragment->WeaponTypeTag;
}

UActiveEquipmentComponent::UActiveEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	EquippedItems.OwnerComponent = this;
}

void UActiveEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority()) return;

	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UEnchantService* Enchant = GI->GetSubsystem<UEnchantService>())
		{
			EnchantedHandle = Enchant->OnItemEnchanted.AddUObject(this, &UActiveEquipmentComponent::HandleItemEnchanted);
		}
	}
}

void UActiveEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EnchantedHandle.IsValid())
	{
		if (UGameInstance* GI = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UEnchantService* Enchant = GI->GetSubsystem<UEnchantService>())
			{
				Enchant->OnItemEnchanted.Remove(EnchantedHandle);
			}
		}
		EnchantedHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void UActiveEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UActiveEquipmentComponent, EquippedItems, Params);
}

UEquipmentInstance* UActiveEquipmentComponent::EquipItem(const FInventoryEntry& Entry)
{
	if (!GetOwner()->HasAuthority()) return nullptr;

	UItemDefinition* ItemDefinition = Entry.Definition.LoadSynchronous();
	if (!IsValid(ItemDefinition)) return nullptr;

	const UItemFragment_Equippable* EquippableFragment = ItemDefinition->FindFragment<UItemFragment_Equippable>();
	if (EquippableFragment == nullptr) return nullptr;

	const FGameplayTag SlotTag = EquippableFragment->SlotTag;
	if (!SlotTag.IsValid()) return nullptr;

	UnequipItem(SlotTag);

	APawn* Pawn = Cast<APawn>(GetOwner());

	UEquipmentInstance* NewInstance = NewObject<UEquipmentInstance>(GetOwner());
	NewInstance->Initialize(Entry.InstanceId, Entry.Definition);
	NewInstance->OnEquipped(Pawn);
	ApplyAbilitySetsFromEntry(NewInstance, Entry);

	const FGameplayTag WeaponTypeTag = GetWeaponTypeTag(ItemDefinition);
	if (WeaponTypeTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			ASC->AddLooseGameplayTag(WeaponTypeTag, 1, EGameplayTagReplicationState::TagOnly);
		}
	}

	FEquipmentEntry NewEntry;
	NewEntry.SlotTag = SlotTag;
	NewEntry.Instance = NewInstance;

	FEquipmentEntry& AddedEntry = EquippedItems.Entries.Add_GetRef(NewEntry);
	EquippedItems.MarkItemDirty(AddedEntry);
	MARK_PROPERTY_DIRTY_FROM_NAME(UActiveEquipmentComponent, EquippedItems, this);

	AddReplicatedSubObject(NewInstance);

	return NewInstance;
}

bool UActiveEquipmentComponent::UnequipItem(FGameplayTag SlotTag)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!SlotTag.IsValid()) return false;

	const int32 Index = EquippedItems.Entries.IndexOfByPredicate([&SlotTag](const FEquipmentEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	if (Index == INDEX_NONE) return false;

	UEquipmentInstance* Instance = EquippedItems.Entries[Index].Instance;
	APawn* Pawn = Cast<APawn>(GetOwner());

	if (IsValid(Instance))
	{
		RevokeAbilitySets(Instance);
		Instance->OnUnequipped(Pawn);
		RemoveReplicatedSubObject(Instance);

		const FGameplayTag WeaponTypeTag = GetWeaponTypeTag(Instance->GetItemDefinition());
		if (WeaponTypeTag.IsValid())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
			{
				ASC->RemoveLooseGameplayTag(WeaponTypeTag, 1, EGameplayTagReplicationState::TagOnly);
			}
		}
	}

	EquippedItems.Entries.RemoveAt(Index);
	EquippedItems.MarkArrayDirty();
	MARK_PROPERTY_DIRTY_FROM_NAME(UActiveEquipmentComponent, EquippedItems, this);

	return true;
}

UEquipmentInstance* UActiveEquipmentComponent::GetEquippedInstance(FGameplayTag SlotTag) const
{
	const FEquipmentEntry* Found = EquippedItems.Entries.FindByPredicate([&SlotTag](const FEquipmentEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	return Found != nullptr ? Found->Instance : nullptr;
}

void UActiveEquipmentComponent::RefreshEquipment(const FInventoryEntry& Entry)
{
	if (!GetOwner()->HasAuthority()) return;

	FEquipmentEntry* Found = EquippedItems.Entries.FindByPredicate([&Entry](const FEquipmentEntry& E)
	{
		return IsValid(E.Instance) && E.Instance->GetInstanceId() == Entry.InstanceId;
	});

	if (Found == nullptr) return;

	UEquipmentInstance* Instance = Found->Instance;
	if (!IsValid(Instance)) return;

	RevokeAbilitySets(Instance);
	ApplyAbilitySetsFromEntry(Instance, Entry);

	EquippedItems.MarkItemDirty(*Found);
	MARK_PROPERTY_DIRTY_FROM_NAME(UActiveEquipmentComponent, EquippedItems, this);
}

void UActiveEquipmentComponent::ApplyAbilitySetsFromEntry(UEquipmentInstance* Instance, const FInventoryEntry& Entry)
{
	if (!IsValid(Instance)) return;

	UItemDefinition* Def = Entry.Definition.LoadSynchronous();
	if (!IsValid(Def)) return;

	APawn* Pawn = Cast<APawn>(GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!IsValid(ASC)) return;

	const UItemFragment_GrantedAbilitySet* GrantFragment = Def->FindFragment<UItemFragment_GrantedAbilitySet>();
	if (GrantFragment != nullptr && IsValid(GrantFragment->AbilitySet))
	{
		GrantFragment->AbilitySet->GiveToAbilitySystem(
			ASC,
			&Instance->GetMutableGrantedHandles(),
			Instance);
	}

	ApplyWeaponBaseStats(Instance, Def, ASC);
	ApplyEnchantOptions(Instance, Entry, ASC);

	// TODO: SetByCaller(Stat.Modifier.Deviation = 1 + Entry.StatDeviation) 주입 — Template GE 인프라 후
	// TODO: Entry.SocketedGemInstanceIds 순회 → 각 Gem의 AbilitySet 부여 — GemSocketService 후
	// TODO: Entry.EnhancementLevel > 0 → 강화 GE 적용 — EnhancementService + Curve 후
	// TODO: ApplyMasteryPenaltyIfNeeded — MasteryComponent (character 도메인) 후
}

void UActiveEquipmentComponent::ApplyEnchantOptions(UEquipmentInstance* Instance, const FInventoryEntry& Entry, UAbilitySystemComponent* ASC)
{
	if (Entry.RolledOptions.IsEmpty()) return;

	const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();
	UDataTable* EffectTable = IsValid(Settings) ? Settings->MagnitudeEffectTable.LoadSynchronous() : nullptr;
	if (!IsValid(EffectTable)) return;

	// 매그니튜드 단위로 분기. 매핑(DT_EnchantMagnitudeEffect) 있는 단순 어트리뷰트 가산은 GE로 즉시 적용,
	// 매핑 없는 매그니튜드(조건부 등)는 히트 시점에 평가되도록 OnHitModifier에 모아 등록.
	TArray<FRolledMagnitude> OnHitModifiers;
	for (const FRolledEnchantOption& Option : Entry.RolledOptions)
	{
		for (const FRolledMagnitude& Magnitude : Option.Magnitudes)
		{
			const FEnchantMagnitudeEffectRow* EffectRow = EffectTable->FindRow<FEnchantMagnitudeEffectRow>(
				Magnitude.MagnitudeTag.GetTagName(), TEXT("ApplyEnchantOptions"), false);
			if (EffectRow == nullptr || !IsValid(EffectRow->Effect))
			{
				OnHitModifiers.Add(Magnitude);
				continue;
			}

			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(Instance);

			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectRow->Effect, 1.f, Context);
			if (!Spec.IsValid()) continue;

			Spec.Data->SetSetByCallerMagnitude(
				GYGameplayTags::Stat_Modifier_OptionMagnitude1, Magnitude.Value * EffectRow->ValueScale);

			const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
			Instance->GetMutableGrantedHandles().GameplayEffectHandles.Add(Handle);

			// 타격 시점에 값이 필요한 매그니튜드(온히트 효과 등)는 GE 적용과 별개로 OnHitModifier에도 등록
			if (EffectRow->bHitTimeValue)
			{
				OnHitModifiers.Add(Magnitude);
			}
		}
	}

	if (!OnHitModifiers.IsEmpty())
	{
		if (UGYOnHitModifierComponent* OnHitComp = GetOwner()->FindComponentByClass<UGYOnHitModifierComponent>())
		{
			OnHitComp->RegisterModifiers(Instance, OnHitModifiers);
		}
	}
}

void UActiveEquipmentComponent::ApplyWeaponBaseStats(UEquipmentInstance* Instance, UItemDefinition* Def, UAbilitySystemComponent* ASC)
{
	const UItemFragment_Weapon* WeaponFragment = Def->FindFragment<UItemFragment_Weapon>();
	if (WeaponFragment == nullptr) return;

	const UGYEquipmentSettings* Settings = GetDefault<UGYEquipmentSettings>();
	if (!IsValid(Settings->BaseATKEffectClass)) return;
	if (Settings->WeaponBaseStatsTable.IsNull()) return;

	UDataTable* Table = Settings->WeaponBaseStatsTable.LoadSynchronous();
	if (!IsValid(Table)) return;

	const FWeaponBaseStatsRow* Row = Table->FindRow<FWeaponBaseStatsRow>(Def->ItemId, TEXT("ApplyWeaponBaseStats"));
	if (Row == nullptr) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Settings->BaseATKEffectClass, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::Stat_Modifier_OptionMagnitude1, Row->BaseATK);

	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	Instance->GetMutableGrantedHandles().GameplayEffectHandles.Add(Handle);
}

void UActiveEquipmentComponent::RevokeAbilitySets(UEquipmentInstance* Instance)
{
	if (!IsValid(Instance)) return;

	if (UGYOnHitModifierComponent* OnHitComp = GetOwner()->FindComponentByClass<UGYOnHitModifierComponent>())
	{
		OnHitComp->UnregisterModifiers(Instance);
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!IsValid(ASC)) return;

	Instance->GetMutableGrantedHandles().TakeFromAbilitySystem(ASC);
}

void UActiveEquipmentComponent::HandleItemEnchanted(FGuid InstanceId)
{
	if (!GetOwner()->HasAuthority()) return;

	const FEquipmentEntry* Found = EquippedItems.Entries.FindByPredicate([&InstanceId](const FEquipmentEntry& E)
	{
		return IsValid(E.Instance) && E.Instance->GetInstanceId() == InstanceId;
	});

	if (Found == nullptr) return;

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!IsValid(Pawn)) return;

	AGYPlayerState* PS = Pawn->GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	const FInventoryEntry* Entry = Inv->FindEntry(InstanceId);
	if (Entry == nullptr) return;

	RefreshEquipment(*Entry);
}

void UActiveEquipmentComponent::OnLoadoutSlotChanged(FGameplayTag SlotTag, FGuid NewInstanceId)
{
	if (!GetOwner()->HasAuthority()) return;

	if (!NewInstanceId.IsValid())
	{
		UnequipItem(SlotTag);
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!IsValid(Pawn)) return;

	AGYPlayerState* PS = Pawn->GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	const FInventoryEntry* Entry = Inv->FindEntry(NewInstanceId);
	if (Entry == nullptr) return;

	EquipItem(*Entry);
}
