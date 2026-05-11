#include "Equipment/EquipmentComponent.h"

#include "AbilitySets/AbilitySet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Equipment/EquipmentInstance.h"
#include "GameFramework/Pawn.h"
#include "Inventory/InventoryEntry.h"
#include "Items/Fragments/ItemFragment_Equippable.h"
#include "Items/Fragments/ItemFragment_GrantedAbilitySet.h"
#include "Items/ItemDefinition.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"

UEquipmentComponent::UEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	EquippedItems.OwnerComponent = this;
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UEquipmentComponent, EquippedItems, Params);
}

UEquipmentInstance* UEquipmentComponent::EquipItem(const FInventoryEntry& Entry)
{
	if (!GetOwner()->HasAuthority()) return nullptr;

	UItemDefinition* Def = Entry.Definition.LoadSynchronous();
	if (!::IsValid(Def)) return nullptr;

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
	MARK_PROPERTY_DIRTY_FROM_NAME(UEquipmentComponent, EquippedItems, this);

	AddReplicatedSubObject(NewInstance);

	OnEquipmentChanged.Broadcast(SlotTag, NewInstance);

	return NewInstance;
}

bool UEquipmentComponent::UnequipItem(FGameplayTag SlotTag)
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

	if (::IsValid(Instance))
	{
		RevokeAbilitySets(Instance);
		Instance->OnUnequipped(Pawn);
		RemoveReplicatedSubObject(Instance);
	}

	EquippedItems.Entries.RemoveAt(Index);
	EquippedItems.MarkArrayDirty();
	MARK_PROPERTY_DIRTY_FROM_NAME(UEquipmentComponent, EquippedItems, this);

	OnEquipmentChanged.Broadcast(SlotTag, nullptr);

	return true;
}

UEquipmentInstance* UEquipmentComponent::GetEquippedInstance(FGameplayTag SlotTag) const
{
	const FEquipmentEntry* Found = EquippedItems.Entries.FindByPredicate([&SlotTag](const FEquipmentEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	return Found != nullptr ? Found->Instance : nullptr;
}

void UEquipmentComponent::RefreshEquipment(const FInventoryEntry& Entry)
{
	if (!GetOwner()->HasAuthority()) return;

	FEquipmentEntry* Found = EquippedItems.Entries.FindByPredicate([&Entry](const FEquipmentEntry& E)
	{
		return ::IsValid(E.Instance) && E.Instance->GetInstanceId() == Entry.InstanceId;
	});

	if (Found == nullptr) return;

	UEquipmentInstance* Instance = Found->Instance;
	if (!::IsValid(Instance)) return;

	RevokeAbilitySets(Instance);
	ApplyAbilitySetsFromEntry(Instance, Entry);

	EquippedItems.MarkItemDirty(*Found);
	MARK_PROPERTY_DIRTY_FROM_NAME(UEquipmentComponent, EquippedItems, this);

	OnEquipmentChanged.Broadcast(Found->SlotTag, Instance);
}

void UEquipmentComponent::ApplyAbilitySetsFromEntry(UEquipmentInstance* Instance, const FInventoryEntry& Entry)
{
	if (!::IsValid(Instance)) return;

	UItemDefinition* Def = Entry.Definition.LoadSynchronous();
	if (!::IsValid(Def)) return;

	APawn* Pawn = Cast<APawn>(GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!::IsValid(ASC)) return;

	const UItemFragment_GrantedAbilitySet* GrantFragment = Def->FindFragment<UItemFragment_GrantedAbilitySet>();
	if (GrantFragment != nullptr && ::IsValid(GrantFragment->AbilitySet))
	{
		GrantFragment->AbilitySet->GiveToAbilitySystem(
			ASC,
			&Instance->GetMutableGrantedHandles(),
			Instance);
	}

	// TODO: SetByCaller(Stat.Modifier.Deviation = 1 + Entry.StatDeviation) 주입 — Template GE 인프라 후
	// TODO: Entry.SocketedGemInstanceIds 순회 → 각 Gem의 AbilitySet 부여 — GemSocketService 후
	// TODO: Entry.OptionIds 순회 → DT_OptionCatalog → Template GE 적용 — Option 카탈로그 후
	// TODO: Entry.EnhancementLevel > 0 → 강화 GE 적용 — EnhancementService + Curve 후
	// TODO: ApplyMasteryPenaltyIfNeeded — MasteryComponent (character 도메인) 후
}

void UEquipmentComponent::RevokeAbilitySets(UEquipmentInstance* Instance)
{
	if (!::IsValid(Instance)) return;

	APawn* Pawn = Cast<APawn>(GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!::IsValid(ASC)) return;

	Instance->GetMutableGrantedHandles().TakeFromAbilitySystem(ASC);
}
