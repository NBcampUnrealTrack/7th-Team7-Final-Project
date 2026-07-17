#include "Equipment/ActiveEquipmentComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystem/AbilitySet.h"
#include "AbilitySystem/AbilitySetGrantedHandles.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/GYOnHitModifierComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/OptionTags.h"
#include "Enchant/EnchantMagnitudeEffectRow.h"
#include "Enchant/EnchantService.h"
#include "Enchant/GYEnchantSettings.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Equipment/EquipmentInstance.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Equipment/GYEquipmentSettings.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ArmorBaseStatsRow.h"
#include "Items/Fragments/ItemFragment_Armor.h"
#include "Items/Fragments/ItemFragment_Equippable.h"
#include "Items/Fragments/ItemFragment_GrantedAbilitySet.h"
#include "Items/Fragments/ItemFragment_Weapon.h"
#include "Items/ItemDefinition.h"
#include "Items/WeaponBaseStatsRow.h"
#include "Logging/GYLogManager.h"
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

void UActiveEquipmentComponent::InitializeLoadoutBinding()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!OwningPawn) return;

	AGYPlayerState* PS = OwningPawn->GetPlayerState<AGYPlayerState>();
	if (!IsValid(PS)) return;

	CachedASC = PS->GetGYAbilitySystemComponent();

	UEquipmentLoadoutComponent* Loadout = PS->GetEquipmentLoadoutComponent();
	if (!IsValid(Loadout)) return;

	Loadout->OnLoadoutSlotChanged.AddUObject(this, &UActiveEquipmentComponent::OnLoadoutSlotChanged);
	BoundLoadout = Loadout;

	// 현재 로드아웃으로 초기 동기화.
	for (const FEquipmentLoadoutEntry& Entry : Loadout->GetEntries())
	{
		OnLoadoutSlotChanged(Entry.SlotTag, Entry.InstanceId);
	}
}

void UActiveEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		for (const FEquipmentEntry& Entry : EquippedItems.Entries)
		{
			if (IsValid(Entry.Instance))
			{
				RevokeAbilitySets(Entry.Instance, Entry.SlotTag);
			}
		}
	}

	RemoveAllVisuals();

	if (BoundLoadout.IsValid())
	{
		BoundLoadout->OnLoadoutSlotChanged.RemoveAll(this);
		BoundLoadout.Reset();
	}

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
	if (EquippableFragment == nullptr)
	{
		GY_WARN(Content, KDY, "장착 실패: %s에 Equippable fragment가 없음", *ItemDefinition->GetName());
		return nullptr;
	}

	const FGameplayTag SlotTag = EquippableFragment->SlotTag;
	if (!SlotTag.IsValid())
	{
		GY_WARN(Content, KDY, "장착 실패: %s의 SlotTag가 비어 있음", *ItemDefinition->GetName());
		return nullptr;
	}

	UnequipItem(SlotTag);

	APawn* Pawn = Cast<APawn>(GetOwner());

	UEquipmentInstance* NewInstance = NewObject<UEquipmentInstance>(GetOwner());
	NewInstance->Initialize(Entry.InstanceId, Entry.Definition);
	NewInstance->OnEquipped(Pawn);
	ApplyAbilitySetsFromEntry(NewInstance, Entry, SlotTag);

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
		RevokeAbilitySets(Instance, SlotTag);
		Instance->OnUnequipped(Pawn);
		RemoveReplicatedSubObject(Instance);
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

void UActiveEquipmentComponent::RemoveAllVisuals()
{
	GY_WARN(Game, KHB, "RemoveAllVisuals 호출, Entries=%d", EquippedItems.Entries.Num());
	APawn* Pawn = Cast<APawn>(GetOwner());
	for (const FEquipmentEntry& Entry : EquippedItems.Entries)
	{
		GY_WARN(Game, KHB, "Entry.Instance valid=%d", IsValid(Entry.Instance));
		if (IsValid(Entry.Instance))
		{
			Entry.Instance->OnUnequipped(Pawn);
		}
	}

	if (AActor* Owner = Pawn)
	{
		GY_WARN(Game, KHB, "붙어잇는 액터 지우기")
		TArray<AActor*> AttachedActors;
		Owner->GetAttachedActors(AttachedActors);
		for (AActor* AttachedActor : AttachedActors)
		{
			if (IsValid(AttachedActor))
			{
				AttachedActor->Destroy();
			}
		}
	}
}

void UActiveEquipmentComponent::MulticastRemoveAllVisuals_Implementation()
{
	GY_WARN(Game, KHB, "Multicast 도착 (Role=%d)", (int32)GetOwnerRole());
	RemoveAllVisuals();
}

void UActiveEquipmentComponent::ReapplyAnimLayers()
{
	for (const FEquipmentEntry& Entry : EquippedItems.Entries)
	{
		if (IsValid(Entry.Instance))
		{
			Entry.Instance->ReapplyAnimLayer();
		}
	}
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

	RevokeAbilitySets(Instance, Found->SlotTag);
	ApplyAbilitySetsFromEntry(Instance, Entry, Found->SlotTag);

	EquippedItems.MarkItemDirty(*Found);
	MARK_PROPERTY_DIRTY_FROM_NAME(UActiveEquipmentComponent, EquippedItems, this);
}

void UActiveEquipmentComponent::ApplyAbilitySetsFromEntry(UEquipmentInstance* Instance, const FInventoryEntry& Entry, FGameplayTag SlotTag)
{
	if (!IsValid(Instance)) return;

	UItemDefinition* Def = Entry.Definition.LoadSynchronous();
	if (!IsValid(Def)) return;

	UGYAbilitySystemComponent* ASC = CachedASC.Get();
	if (!IsValid(ASC)) return;

	const FGameplayTag WeaponTypeTag = GetWeaponTypeTag(Def);
	if (WeaponTypeTag.IsValid())
	{
		ASC->Grant_AddLooseTag(SlotTag, WeaponTypeTag, 1, EGameplayTagReplicationState::TagOnly);
	}

	const UItemFragment_GrantedAbilitySet* GrantFragment = Def->FindFragment<UItemFragment_GrantedAbilitySet>();
	if (GrantFragment != nullptr && IsValid(GrantFragment->AbilitySet))
	{
		FAbilitySetGrantedHandles Temp;
		GrantFragment->AbilitySet->GiveToAbilitySystem(ASC, &Temp, Instance);
		ASC->Grant_AdoptHandles(SlotTag, Temp);
	}

	ApplyWeaponBaseStats(Instance, Def, ASC, SlotTag);
	ApplyArmorBaseStats(Instance, Def, ASC, SlotTag);
	ApplyEnchantOptions(Instance, Entry, ASC, SlotTag);

	// TODO: SetByCaller(Stat.Modifier.Deviation = 1 + Entry.StatDeviation) 주입 — Template GE 인프라 후
	// TODO: Entry.SocketedGemInstanceIds 순회 → 각 Gem의 AbilitySet 부여 — GemSocketService 후
	// TODO: Entry.EnhancementLevel > 0 → 강화 GE 적용 — EnhancementService + Curve 후
	// TODO: ApplyMasteryPenaltyIfNeeded — MasteryComponent (character 도메인) 후
}

void UActiveEquipmentComponent::ApplyEnchantOptions(UEquipmentInstance* Instance, const FInventoryEntry& Entry, UGYAbilitySystemComponent* ASC, FGameplayTag SlotTag)
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

			ASC->Grant_ApplyEffectSpec(SlotTag, *Spec.Data);

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

void UActiveEquipmentComponent::ApplyWeaponBaseStats(UEquipmentInstance* Instance, UItemDefinition* Def, UGYAbilitySystemComponent* ASC, FGameplayTag SlotTag)
{
	const UItemFragment_Weapon* WeaponFragment = Def->FindFragment<UItemFragment_Weapon>();
	if (WeaponFragment == nullptr) return;

	const UGYEquipmentSettings* Settings = GetDefault<UGYEquipmentSettings>();
	if (Settings->WeaponBaseStatsTable.IsNull()) return;

	UDataTable* Table = Settings->WeaponBaseStatsTable.LoadSynchronous();
	if (!IsValid(Table)) return;

	const FWeaponBaseStatsRow* Row = Table->FindRow<FWeaponBaseStatsRow>(Def->ItemId, TEXT("ApplyWeaponBaseStats"));
	if (Row == nullptr)
	{
		GY_WARN(Content, KDY, "%s: %s에 ItemId '%s' 행이 없어 무기 베이스 스탯이 적용되지 않음", *Def->GetName(), *Table->GetName(), *Def->ItemId.ToString());
		return;
	}

	ApplyFlatStatEffect(Instance, ASC, Settings->BaseATKEffectClass, Row->BaseATK, SlotTag);
}

void UActiveEquipmentComponent::ApplyArmorBaseStats(UEquipmentInstance* Instance, UItemDefinition* Def, UGYAbilitySystemComponent* ASC, FGameplayTag SlotTag)
{
	const UItemFragment_Armor* ArmorFragment = Def->FindFragment<UItemFragment_Armor>();
	if (ArmorFragment == nullptr) return;

	const UGYEquipmentSettings* Settings = GetDefault<UGYEquipmentSettings>();
	if (Settings->ArmorBaseStatsTable.IsNull()) return;

	UDataTable* Table = Settings->ArmorBaseStatsTable.LoadSynchronous();
	if (!IsValid(Table)) return;

	const FArmorBaseStatsRow* Row = Table->FindRow<FArmorBaseStatsRow>(Def->ItemId, TEXT("ApplyArmorBaseStats"));
	if (Row == nullptr)
	{
		GY_WARN(Content, KDY, "%s: %s에 ItemId '%s' 행이 없어 방어구 베이스 스탯이 적용되지 않음", *Def->GetName(), *Table->GetName(), *Def->ItemId.ToString());
		return;
	}

	ApplyFlatStatEffect(Instance, ASC, Settings->BaseDEFEffectClass, Row->BaseDEF, SlotTag);
	ApplyFlatStatEffect(Instance, ASC, Settings->BaseMaxHPEffectClass, Row->BaseHP, SlotTag);
}

void UActiveEquipmentComponent::ApplyFlatStatEffect(UEquipmentInstance* Instance, UGYAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> EffectClass, float Value, FGameplayTag SlotTag)
{
	if (!IsValid(EffectClass)) return;
	if (Value == 0.f) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Instance);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::Stat_Modifier_OptionMagnitude1, Value);

	ASC->Grant_ApplyEffectSpec(SlotTag, *Spec.Data);
}

void UActiveEquipmentComponent::RevokeAbilitySets(UEquipmentInstance* Instance, FGameplayTag SlotTag)
{
	if (!IsValid(Instance)) return;

	if (UGYOnHitModifierComponent* OnHitComp = GetOwner()->FindComponentByClass<UGYOnHitModifierComponent>())
	{
		OnHitComp->UnregisterModifiers(Instance);
	}

	UGYAbilitySystemComponent* ASC = CachedASC.Get();
	if (!IsValid(ASC)) return;

	ASC->RevokeGrantSource(SlotTag);
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
