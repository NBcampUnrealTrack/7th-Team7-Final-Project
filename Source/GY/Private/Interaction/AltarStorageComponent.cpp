#include "Interaction/AltarStorageComponent.h"

#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Currency/CurrencyComponent.h"
#include "Disassemble/DisassembleService.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Items/ItemDefinition.h"
#include "Items/Fragments/ItemFragment_Stackable.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

namespace
{
	// 제단은 분해 제물 전용 — 장비 카테고리만 받는다 (포션 등 비장비 차단).
	bool IsAltarEligible(const TSoftObjectPtr<UItemDefinition>& Def)
	{
		const UItemDefinition* DefPtr = Def.LoadSynchronous();
		return IsValid(DefPtr) && DefPtr->CategoryTags.HasTag(GYGameplayTags::Item_Category_Equipment);
	}
}


UAltarStorageComponent::UAltarStorageComponent()
{
	SetIsReplicatedByDefault(true);
	Storage.OwnerComponent = this;
	Capacity = 20;
}

void UAltarStorageComponent::BeginPlay()
{
	Super::BeginPlay();
	Storage.OwnerComponent = this;
}


void UAltarStorageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UAltarStorageComponent, Storage, Params);
}

int32 UAltarStorageComponent::TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId)
{
	if (!GetOwner()->HasAuthority()) return 0;
	if (Count <= 0) return 0;
	if (Def.IsNull()) return 0;
	if (!IsAltarEligible(Def)) return 0;

	UItemDefinition* DefPtr = Def.LoadSynchronous();
	if (!IsValid(DefPtr)) return 0;

	const UItemFragment_Stackable* StackableFragment = DefPtr->FindFragment<UItemFragment_Stackable>();
	const int32 MaxStack = StackableFragment != nullptr ? StackableFragment->MaxStackSize : 1;

	int32 Remaining = Count;
	OutInstanceId = FGuid();

	// 1) 같은 Definition의 기존 스택에 채우기
	if (MaxStack > 1)
	{
		for (FInventoryEntry& Entry : Storage.Entries)
		{
			if (Remaining <= 0) break;
			if (Entry.Definition != Def) continue;
			if (Entry.StackCount >= MaxStack) continue;

			const int32 Space = MaxStack - Entry.StackCount;
			const int32 ToAdd = FMath::Min(Space, Remaining);
			Entry.StackCount += ToAdd;
			Remaining -= ToAdd;

			Storage.MarkItemDirty(Entry);
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

		FInventoryEntry& AddedEntry = Storage.Entries.Add_GetRef(NewEntry);
		Storage.MarkItemDirty(AddedEntry);
		Remaining -= ToAdd;

		if (!OutInstanceId.IsValid())
		{
			OutInstanceId = AddedEntry.InstanceId;
		}

		NotifyContainerChanged(AddedEntry.InstanceId, EInventoryEventType::Added);
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UAltarStorageComponent, Storage, this);

	return Count - Remaining;
}

bool UAltarStorageComponent::TryRemoveItem(const FGuid& InstanceId, int32 Count)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (Count <= 0) return false;

	const int32 Index = Storage.Entries.IndexOfByPredicate([&InstanceId](const FInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});

	if (Index == INDEX_NONE) return false;

	FInventoryEntry& Entry = Storage.Entries[Index];
	Entry.StackCount -= Count;

	EInventoryEventType EventType = EInventoryEventType::StackCountChanged;

	if (Entry.StackCount <= 0)
	{
		Storage.Entries.RemoveAt(Index);
		Storage.MarkArrayDirty();
		EventType = EInventoryEventType::Removed;
	}
	else
	{
		Storage.MarkItemDirty(Entry);
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UAltarStorageComponent, Storage, this);

	NotifyContainerChanged(InstanceId, EventType);

	return true;
}

bool UAltarStorageComponent::MutateEntry(const FGuid& InstanceId, TFunctionRef<void(FInventoryEntry&)> Mutator)
{
	if (!GetOwner()->HasAuthority()) return false;

	FInventoryEntry* Entry = Storage.Entries.FindByPredicate([&InstanceId](const FInventoryEntry& E)
	{
		return E.InstanceId == InstanceId;
	});

	if (Entry == nullptr) return false;

	Mutator(*Entry);
	Storage.MarkItemDirty(*Entry);

	MARK_PROPERTY_DIRTY_FROM_NAME(UAltarStorageComponent, Storage, this);

	NotifyContainerChanged(InstanceId, EInventoryEventType::Mutated);

	return true;
}

const FInventoryEntry* UAltarStorageComponent::FindEntry(const FGuid& InstanceId) const
{
	return Storage.Entries.FindByPredicate([&InstanceId](const FInventoryEntry& Entry)
	{
		return Entry.InstanceId == InstanceId;
	});
}

void UAltarStorageComponent::NotifyContainerChanged(const FGuid& InstanceId, EInventoryEventType EventType)
{
	OnAltarChanged.Broadcast(InstanceId, EventType);

	UWorld* World = GetWorld();
	if (World != nullptr && !World->IsNetMode(NM_DedicatedServer))
	{
		FGYInventoryEntryMessage Msg;
		Msg.InstanceId = InstanceId;
		Msg.EventType = EventType;
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Altar_EntryChanged, Msg);
	}
}


bool UAltarStorageComponent::InsertEntry(const FInventoryEntry& Entry)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!IsAltarEligible(Entry.Definition)) return false;

	FInventoryEntry& Added = Storage.Entries.Add_GetRef(Entry);
	Storage.MarkItemDirty(Added);
	MARK_PROPERTY_DIRTY_FROM_NAME(UAltarStorageComponent, Storage, this);

	NotifyContainerChanged(Added.InstanceId, EInventoryEventType::Added);
	return true;
}

bool UAltarStorageComponent::TakeEntry(const FGuid& InstanceId, FInventoryEntry& OutEntry)
{
	if (!GetOwner()->HasAuthority()) return false;

	const int32 Index = Storage.Entries.IndexOfByPredicate(
		[&InstanceId](const FInventoryEntry& E) { return E.InstanceId == InstanceId; });
	if (Index == INDEX_NONE) return false;

	OutEntry = Storage.Entries[Index];
	const FGuid RemovedId = OutEntry.InstanceId;

	Storage.Entries.RemoveAt(Index);
	Storage.MarkArrayDirty();
	MARK_PROPERTY_DIRTY_FROM_NAME(UAltarStorageComponent, Storage, this);

	NotifyContainerChanged(RemovedId, EInventoryEventType::Removed);
	return true;
}


void UAltarStorageComponent::Server_RequestDisassemble_Implementation()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwner());
	if (!IsValid(PS)) return;

	UCurrencyComponent* Currency = PS->GetCurrencyComponent();
	if (!IsValid(Currency)) return;

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!IsValid(GI)) return;

	UDisassembleService* Disassemble = GI->GetSubsystem<UDisassembleService>();
	if (!IsValid(Disassemble)) return;

	TArray<FGuid> InstanceIds;
	InstanceIds.Reserve(Storage.Entries.Num());
	for (const FInventoryEntry& Entry : Storage.Entries)
	{
		InstanceIds.Add(Entry.InstanceId);
	}

	for (const FGuid& InstanceId : InstanceIds)
	{
		Disassemble->TryDisassemble(this, Currency, InstanceId);
	}
}

int32 UAltarStorageComponent::GetCapacity() const
{
	return Capacity;
}

FGameplayTag UAltarStorageComponent::GetContainerTag() const
{
	return GYGameplayTags::Container_Altar;
}
