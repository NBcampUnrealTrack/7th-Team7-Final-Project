#include "Inventory/InventoryComponent.h"

#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Currency/CurrencyComponent.h"
#include "Disassemble/DisassembleService.h"
#include "Enchant/EnchantService.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Items/Fragments/ItemFragment_Consumable.h"
#include "Items/ItemDefinition.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	Inventory.OwnerComponent = this;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryComponent, Inventory, Params);
}

bool UInventoryComponent::TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (Count <= 0) return false;
	if (Def.IsNull()) return false;

	FInventoryEntry NewEntry;
	NewEntry.InstanceId = FGuid::NewGuid();
	NewEntry.Definition = Def;
	NewEntry.StackCount = Count;

	FInventoryEntry& AddedEntry = Inventory.Entries.Add_GetRef(NewEntry);
	Inventory.MarkItemDirty(AddedEntry);

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	OutInstanceId = AddedEntry.InstanceId;

	NotifyInventoryChanged(AddedEntry.InstanceId, EInventoryEventType::Added);

	return true;
}

bool UInventoryComponent::TryRemoveItem(const FGuid& InstanceId, int32 Count)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (Count <= 0) return false;

	const int32 Index = Inventory.Entries.IndexOfByPredicate([&InstanceId](const FInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});

	if (Index == INDEX_NONE) return false;

	FInventoryEntry& Entry = Inventory.Entries[Index];
	Entry.StackCount -= Count;

	EInventoryEventType EventType = EInventoryEventType::StackCountChanged;

	if (Entry.StackCount <= 0)
	{
		Inventory.Entries.RemoveAt(Index);
		Inventory.MarkArrayDirty();
		EventType = EInventoryEventType::Removed;
	}
	else
	{
		Inventory.MarkItemDirty(Entry);
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	NotifyInventoryChanged(InstanceId, EventType);

	return true;
}

bool UInventoryComponent::MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator)
{
	if (!GetOwner()->HasAuthority()) return false;

	FInventoryEntry* Entry = Inventory.Entries.FindByPredicate([&InstanceId](const FInventoryEntry& E)
	{
		return E.InstanceId == InstanceId;
	});

	if (Entry == nullptr) return false;

	Mutator(*Entry);
	Inventory.MarkItemDirty(*Entry);

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	NotifyInventoryChanged(InstanceId, EInventoryEventType::Mutated);

	return true;
}

const FInventoryEntry* UInventoryComponent::FindEntry(const FGuid& InstanceId) const
{
	return Inventory.Entries.FindByPredicate([&InstanceId](const FInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});
}

void UInventoryComponent::Server_RequestEnchant_Implementation(const FGuid& InstanceId)
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwner());
	if (!IsValid(PS)) return;

	UCurrencyComponent* Currency = PS->GetCurrencyComponent();
	if (!IsValid(Currency)) return;

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!IsValid(GI)) return;

	UEnchantService* Enchant = GI->GetSubsystem<UEnchantService>();
	if (!IsValid(Enchant)) return;

	FRandomStream Seed;
	Seed.GenerateNewSeed();

	TArray<FRolledEnchantOption> Rolled;
	Enchant->TryEnchant(this, Currency, InstanceId, Seed, Rolled);
}

void UInventoryComponent::Server_RequestDisassemble_Implementation(const FGuid& InstanceId)
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwner());
	if (!IsValid(PS)) return;

	UCurrencyComponent* Currency = PS->GetCurrencyComponent();
	if (!IsValid(Currency)) return;

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!IsValid(GI)) return;

	UDisassembleService* Disassemble = GI->GetSubsystem<UDisassembleService>();
	if (!IsValid(Disassemble)) return;

	Disassemble->TryDisassemble(this, Currency, InstanceId);
}

void UInventoryComponent::NotifyInventoryChanged(const FGuid& InstanceId, EInventoryEventType EventType)
{
	OnInventoryChanged.Broadcast(InstanceId, EventType);
	BroadcastPotionSnapshots();
}

void UInventoryComponent::BroadcastPotionSnapshots()
{
	UWorld* World = GetWorld();
	if (World == nullptr || World->IsNetMode(NM_DedicatedServer)) return;

	struct FPoolAccum
	{
		TSoftObjectPtr<UTexture2D> Icon;
		int32 StackCount = 0;
	};
	TMap<FGameplayTag, FPoolAccum> Current;

	for (const FInventoryEntry& Entry : Inventory.Entries)
	{
		UItemDefinition* Def = Entry.Definition.LoadSynchronous();
		if (!IsValid(Def)) continue;

		const UItemFragment_Consumable* Consumable = Def->FindFragment<UItemFragment_Consumable>();
		if (Consumable == nullptr || !Consumable->ChargePoolTag.IsValid()) continue;

		FPoolAccum& Accum = Current.FindOrAdd(Consumable->ChargePoolTag);
		if (Accum.StackCount == 0)
		{
			Accum.Icon = Def->Icon;
		}
		Accum.StackCount += Entry.StackCount;
	}

	UGameplayMessageSubsystem& MS = UGameplayMessageSubsystem::Get(World);

	TSet<FGameplayTag> CurrentKeys;
	Current.GetKeys(CurrentKeys);

	for (const TPair<FGameplayTag, FPoolAccum>& Pair : Current)
	{
		FGYPotionSlotMessage Msg;
		Msg.ChargePoolTag = Pair.Key;
		Msg.Icon = Pair.Value.Icon;
		Msg.StackCount = Pair.Value.StackCount;
		Msg.bIsEmpty = Pair.Value.StackCount <= 0;
		MS.BroadcastMessage(GYGameplayTags::Message_Inventory_PotionSlotChanged, Msg);
	}

	for (const FGameplayTag& VanishedTag : LastPublishedPotionTags.Difference(CurrentKeys))
	{
		FGYPotionSlotMessage Msg;
		Msg.ChargePoolTag = VanishedTag;
		Msg.StackCount = 0;
		Msg.bIsEmpty = true;
		MS.BroadcastMessage(GYGameplayTags::Message_Inventory_PotionSlotChanged, Msg);
	}

	LastPublishedPotionTags = MoveTemp(CurrentKeys);
}

TArray<FInventoryEntry> UInventoryComponent::GetAllEntriesByCategory(FGameplayTag CategoryTag) const
{
	TArray<FInventoryEntry> Result;

	for (const FInventoryEntry& Entry : Inventory.Entries)
	{
		const UItemDefinition* Def = Entry.Definition.LoadSynchronous();
		if (IsValid(Def) && Def->CategoryTags.HasTag(CategoryTag))
		{
			Result.Add(Entry);
		}
	}

	return Result;
}
