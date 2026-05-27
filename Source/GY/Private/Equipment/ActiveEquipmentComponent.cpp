#include "Equipment/ActiveEquipmentComponent.h"

#include "AbilitySystem/AbilitySet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/OptionTags.h"
#include "Enchant/EnchantOptionResolver.h"
#include "Enchant/EnchantService.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Equipment/EquipmentInstance.h"
#include "Equipment/GYEquipmentSettings.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/EnchantOptionRow.h"
#include "Items/Fragments/ItemFragment_Equippable.h"
#include "Items/Fragments/ItemFragment_GrantedAbilitySet.h"
#include "Items/Fragments/ItemFragment_Weapon.h"
#include "Items/ItemDefinition.h"
#include "Items/WeaponBaseStatsRow.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"

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

	UItemDefinition* Def = Entry.Definition.LoadSynchronous();
	if (!IsValid(Def)) return nullptr;

	const UItemFragment_Equippable* EquippableFragment = Def->FindFragment<UItemFragment_Equippable>();
	if (EquippableFragment == nullptr) return nullptr;

	const FGameplayTag SlotTag = EquippableFragment->SlotTag;
	if (!SlotTag.IsValid()) return nullptr;

	UnequipItem(SlotTag);

	APawn* Pawn = Cast<APawn>(GetOwner());

	UEquipmentInstance* NewInstance = NewObject<UEquipmentInstance>(GetOwner());
	NewInstance->Initialize(Entry.InstanceId, Entry.Definition);
	NewInstance->OnEquipped(Pawn);
	ApplyAbilitySetsFromEntry(NewInstance, Entry);

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
	ApplyEnchantOptions(Instance, Def, Entry, ASC);

	// TODO: SetByCaller(Stat.Modifier.Deviation = 1 + Entry.StatDeviation) 주입 — Template GE 인프라 후
	// TODO: Entry.SocketedGemInstanceIds 순회 → 각 Gem의 AbilitySet 부여 — GemSocketService 후
	// TODO: Entry.EnhancementLevel > 0 → 강화 GE 적용 — EnhancementService + Curve 후
	// TODO: ApplyMasteryPenaltyIfNeeded — MasteryComponent (character 도메인) 후
}

void UActiveEquipmentComponent::ApplyEnchantOptions(UEquipmentInstance* Instance, UItemDefinition* Def, const FInventoryEntry& Entry, UAbilitySystemComponent* ASC)
{
	if (Entry.RolledOptions.IsEmpty()) return;

	// TODO (combat 도메인): 조건부/프록 옵션(약공 한정·출혈·흡혈 등)은 GE 템플릿으로 표현 불가 → 부여 어빌리티로 분기 필요.
	// 여기선 롤된 수치를 MagnitudeTag 키로 SetByCaller 주입하는 plumbing만 처리.
	for (const FRolledEnchantOption& Option : Entry.RolledOptions)
	{
		const FEnchantOptionRow* Row = EnchantOptionResolver::FindRow(Def, Option.OptionId);
		if (Row == nullptr) continue;
		if (!IsValid(Row->TemplateGE)) continue;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Instance);

		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Row->TemplateGE, 1.f, Context);
		if (!Spec.IsValid()) continue;

		for (const FRolledMagnitude& Magnitude : Option.Magnitudes)
		{
			Spec.Data->SetSetByCallerMagnitude(Magnitude.MagnitudeTag, Magnitude.Value);
		}

		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		Instance->GetMutableGrantedHandles().GameplayEffectHandles.Add(Handle);
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
