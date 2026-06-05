#include "Inventory/InventoryComponent.h"

#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Currency/CurrencyComponent.h"
#include "Disassemble/DisassembleService.h"
#include "Enchant/EnchantService.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Items/Fragments/ItemFragment_Consumable.h"
#include "Items/Fragments/ItemFragment_Stackable.h"
#include "Items/ItemDefinition.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	Inventory.OwnerComponent = this;
	Capacity = 30;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
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


bool UInventoryComponent::RefillConsumable(TSoftObjectPtr<UItemDefinition> Def, int32& Count, FGuid& OutInstanceId, const UItemFragment_Consumable* ConsumableFragment)
{
	const FGameplayTag PoolTag = ConsumableFragment->ChargePoolTag;
	const int32 PoolSum = GetChargePoolSum(PoolTag);
	const int32 Space = FMath::Max(0, ConsumableFragment->MaxCharge - PoolSum);
	Count = FMath::Min(Count, Space);
	if (Count <= 0) return false;

	for (FInventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.Definition != Def) continue;

		Entry.StackCount += Count;
		Inventory.MarkItemDirty(Entry);
		NotifyContainerChanged(Entry.InstanceId, EInventoryEventType::StackCountChanged);

		OutInstanceId = Entry.InstanceId;
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);
		BroadcastPotionSnapshots();
		return true;
	}

	FInventoryEntry NewEntry;
	NewEntry.InstanceId = FGuid::NewGuid();
	NewEntry.Definition = Def;
	NewEntry.StackCount = Count;

	FInventoryEntry& AddedEntry = Inventory.Entries.Add_GetRef(NewEntry);
	Inventory.MarkItemDirty(AddedEntry);

	OutInstanceId = AddedEntry.InstanceId;
	NotifyContainerChanged(AddedEntry.InstanceId, EInventoryEventType::Added);

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);
	BroadcastPotionSnapshots();
	return true;
}

bool UInventoryComponent::TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (Count <= 0) return false;
	if (Def.IsNull()) return false;

	UItemDefinition* DefPtr = Def.LoadSynchronous();
	if (!IsValid(DefPtr)) return false;

	const UItemFragment_Consumable* ConsumableFragment = DefPtr->FindFragment<UItemFragment_Consumable>();

	OutInstanceId = FGuid();

	//포션용
	if (ConsumableFragment && ConsumableFragment->ChargePoolTag.IsValid())
	{
		return RefillConsumable(Def, Count, OutInstanceId, ConsumableFragment);
	}


	const UItemFragment_Stackable* StackableFragment = DefPtr->FindFragment<UItemFragment_Stackable>();
	const int32 MaxStack = StackableFragment != nullptr ? StackableFragment->MaxStackSize : 1;

	int32 Remaining = Count;
	OutInstanceId = FGuid();

	// 1) 같은 Definition의 기존 스택에 채우기
	if (MaxStack > 1)
	{
		for (FInventoryEntry& Entry : Inventory.Entries)
		{
			if (Remaining <= 0) break;
			if (Entry.Definition != Def) continue;
			if (Entry.StackCount >= MaxStack) continue;

			const int32 Space = MaxStack - Entry.StackCount;
			const int32 ToAdd = FMath::Min(Space, Remaining);
			Entry.StackCount += ToAdd;
			Remaining -= ToAdd;

			Inventory.MarkItemDirty(Entry);
			NotifyContainerChanged(Entry.InstanceId, EInventoryEventType::StackCountChanged);

			if (!OutInstanceId.IsValid())
			{
				OutInstanceId = Entry.InstanceId;
			}
		}
	}

	// 2) 남은 수량은 새 엔트리에 (MaxStack 초과면 여러 엔트리로 분배)
	while (Remaining > 0)
	{
		const int32 ToAdd = FMath::Min(MaxStack, Remaining);

		FInventoryEntry NewEntry;
		NewEntry.InstanceId = FGuid::NewGuid();
		NewEntry.Definition = Def;
		NewEntry.StackCount = ToAdd;

		FInventoryEntry& AddedEntry = Inventory.Entries.Add_GetRef(NewEntry);
		Inventory.MarkItemDirty(AddedEntry);
		Remaining -= ToAdd;

		if (!OutInstanceId.IsValid())
		{
			OutInstanceId = AddedEntry.InstanceId;
		}

		NotifyContainerChanged(AddedEntry.InstanceId, EInventoryEventType::Added);
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

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

	NotifyContainerChanged(InstanceId, EventType);

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

	NotifyContainerChanged(InstanceId, EInventoryEventType::Mutated);

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


void UInventoryComponent::NotifyContainerChanged(const FGuid& InstanceId, EInventoryEventType EventType)
{
	OnInventoryChanged.Broadcast(InstanceId, EventType);

	UWorld* World = GetWorld();
	if (World != nullptr && !World->IsNetMode(NM_DedicatedServer))
	{
		FGYInventoryEntryMessage Msg;
		Msg.InstanceId = InstanceId;
		Msg.EventType = EventType;
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Inventory_EntryChanged, Msg);
	}

	BroadcastPotionSnapshots();
}


bool UInventoryComponent::InsertEntry(const FInventoryEntry& Entry)
{
	if (!GetOwner()->HasAuthority()) return false;

	FInventoryEntry& Added = Inventory.Entries.Add_GetRef(Entry);
	Inventory.MarkItemDirty(Added);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	NotifyContainerChanged(Added.InstanceId, EInventoryEventType::Added);
	return true;
}

bool UInventoryComponent::TakeEntry(const FGuid& InstanceId, FInventoryEntry& OutEntry)
{
	if (!GetOwner()->HasAuthority()) return false;

	const int32 Index = Inventory.Entries.IndexOfByPredicate(
		[&InstanceId](const FInventoryEntry& E){ return E.InstanceId == InstanceId; });
	if (Index == INDEX_NONE) return false;

	OutEntry = Inventory.Entries[Index];
	const FGuid RemovedId = OutEntry.InstanceId;

	Inventory.Entries.RemoveAt(Index);
	Inventory.MarkArrayDirty();
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	NotifyContainerChanged(RemovedId, EInventoryEventType::Removed);
	return true;
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

int32 UInventoryComponent::GetChargePoolSum(const FGameplayTag& PoolTag) const
{
	int32 Sum = 0;
	for (const FInventoryEntry& Entry : Inventory.Entries)
	{
		UItemDefinition* Def = Entry.Definition.LoadSynchronous();
		if (!Def) continue;
		const UItemFragment_Consumable* Consumable = Def->FindFragment<UItemFragment_Consumable>();
		if (Consumable && Consumable->ChargePoolTag == PoolTag)
		{
			Sum += Entry.StackCount;
		}
	}
	return Sum;
}


int32 UInventoryComponent::GetCapacity() const
{
	return Capacity;
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

