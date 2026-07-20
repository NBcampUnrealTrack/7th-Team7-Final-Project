#include "Inventory/InventoryComponent.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GameplayTagsManager.h"
#include "UObject/SoftObjectPath.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Core/GameplayTags/SoundTags.h"
#include "Currency/CurrencyComponent.h"
#include "Disassemble/DisassembleService.h"
#include "Enchant/EnchantService.h"
#include "Equipment/EquipmentLoadoutComponent.h"
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
	Capacity = 42;
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


int32 UInventoryComponent::TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId)
{
	return TryAddItem(Def, Count, OutInstanceId, [](FInventoryEntry&) {});
}

int32 UInventoryComponent::TryAddItem(TSoftObjectPtr<UItemDefinition> Def, int32 Count, FGuid& OutInstanceId, TFunctionRef<void(FInventoryEntry&)> InitNewEntry)
{
	if (!GetOwner()->HasAuthority()) return 0;
	if (Count <= 0) return 0;
	if (Def.IsNull()) return 0;

	UItemDefinition* DefPtr = Def.LoadSynchronous();
	if (!IsValid(DefPtr)) return 0;

	OutInstanceId = FGuid();

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

	// 2) 남은 수량은 새 엔트리에 (MaxStack 초과면 여러 엔트리로 분배). 단 빈 슬롯이 있을 때만
	int32 FreeSlots = Capacity - GetOccupiedSlotCount();
	while (Remaining > 0 && FreeSlots > 0)
	{
		const int32 ToAdd = FMath::Min(MaxStack, Remaining);

		FInventoryEntry NewEntry;
		NewEntry.InstanceId = FGuid::NewGuid();
		NewEntry.Definition = Def;
		NewEntry.StackCount = ToAdd;
		InitNewEntry(NewEntry);

		FInventoryEntry& AddedEntry = Inventory.Entries.Add_GetRef(NewEntry);
		Inventory.MarkItemDirty(AddedEntry);
		Remaining -= ToAdd;
		--FreeSlots;

		if (!OutInstanceId.IsValid())
		{
			OutInstanceId = AddedEntry.InstanceId;
		}

		NotifyContainerChanged(AddedEntry.InstanceId, EInventoryEventType::Added);
	}

	const int32 AddedCount = Count - Remaining;
	if (AddedCount > 0)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);
	}

	return AddedCount;
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
	if (Enchant->TryEnchant(this, Currency, InstanceId, Seed, Rolled))
	{
		PS->Client_PlaySound(GYGameplayTags::Sound_Interaction_TimeRift_Enchant);
	}
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
	// 빈 슬롯이 없으면 통째 이동 불가 (드래그·이동 등)
	if (GetOccupiedSlotCount() >= Capacity) return false;

	FInventoryEntry& Added = Inventory.Entries.Add_GetRef(Entry);
	Inventory.MarkItemDirty(Added);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	NotifyContainerChanged(Added.InstanceId, EInventoryEventType::Added);
	return true;
}

TSharedPtr<FJsonValue> UInventoryComponent::ExportSaveData() const
{
	TArray<TSharedPtr<FJsonValue>> Items;

	for (const FInventoryEntry& Entry : Inventory.Entries)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("instanceId"), Entry.InstanceId.ToString(EGuidFormats::Digits));
		Object->SetStringField(TEXT("definition"), Entry.Definition.ToSoftObjectPath().ToString());
		Object->SetNumberField(TEXT("stackCount"), Entry.StackCount);
		Object->SetNumberField(TEXT("level"), Entry.Level);
		Object->SetNumberField(TEXT("enhancement"), Entry.EnhancementLevel);
		Object->SetStringField(TEXT("grade"), Entry.GradeTag.IsValid() ? Entry.GradeTag.ToString() : FString());
		Object->SetNumberField(TEXT("deviation"), Entry.StatDeviation);
		Object->SetNumberField(TEXT("seed"), Entry.RandomSeed);

		// 인첸트 롤 (옵션 → 마그니튜드 2단 중첩)
		TArray<TSharedPtr<FJsonValue>> Options;
		for (const FRolledEnchantOption& Option : Entry.RolledOptions)
		{
			const TSharedRef<FJsonObject> OptionObject = MakeShared<FJsonObject>();
			OptionObject->SetStringField(TEXT("optionId"), Option.OptionId.ToString());

			TArray<TSharedPtr<FJsonValue>> Magnitudes;
			for (const FRolledMagnitude& Magnitude : Option.Magnitudes)
			{
				const TSharedRef<FJsonObject> MagnitudeObject = MakeShared<FJsonObject>();
				MagnitudeObject->SetStringField(TEXT("tag"),
					Magnitude.MagnitudeTag.IsValid() ? Magnitude.MagnitudeTag.ToString() : FString());
				MagnitudeObject->SetNumberField(TEXT("value"), Magnitude.Value);
				Magnitudes.Add(MakeShared<FJsonValueObject>(MagnitudeObject));
			}
			OptionObject->SetArrayField(TEXT("magnitudes"), Magnitudes);
			Options.Add(MakeShared<FJsonValueObject>(OptionObject));
		}
		Object->SetArrayField(TEXT("rolledOptions"), Options);

		// 소켓 젬 (다른 인벤 아이템의 InstanceId 참조)
		TArray<TSharedPtr<FJsonValue>> Sockets;
		for (const FGuid& GemId : Entry.SocketedGemInstanceIds)
		{
			Sockets.Add(MakeShared<FJsonValueString>(GemId.ToString(EGuidFormats::Digits)));
		}
		Object->SetArrayField(TEXT("sockets"), Sockets);

		Items.Add(MakeShared<FJsonValueObject>(Object));
	}

	return MakeShared<FJsonValueArray>(Items);
}

void UInventoryComponent::ImportSaveData(const TSharedPtr<FJsonValue>& Data)
{
	if (!GetOwner()->HasAuthority()) return;
	if (!Data.IsValid()) return;

	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (!Data->TryGetArray(Items) || Items == nullptr) return;

	// 기존 인벤 비우고 세이브 항목으로 재구축
	Inventory.Entries.Reset();
	Inventory.MarkArrayDirty();
	MARK_PROPERTY_DIRTY_FROM_NAME(UInventoryComponent, Inventory, this);

	for (const TSharedPtr<FJsonValue>& ItemValue : *Items)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!ItemValue->TryGetObject(Object) || Object == nullptr) continue;

		FInventoryEntry Entry;
		FString StringValue;
		int32 IntValue = 0;
		double DoubleValue = 0.0;

		if ((*Object)->TryGetStringField(TEXT("instanceId"), StringValue))
		{
			FGuid::Parse(StringValue, Entry.InstanceId);
		}
		if ((*Object)->TryGetStringField(TEXT("definition"), StringValue))
		{
			Entry.Definition = TSoftObjectPtr<UItemDefinition>(FSoftObjectPath(StringValue));
		}
		if ((*Object)->TryGetNumberField(TEXT("stackCount"), IntValue))
		{
			Entry.StackCount = IntValue;
		}
		if ((*Object)->TryGetNumberField(TEXT("level"), IntValue))
		{
			Entry.Level = IntValue;
		}
		if ((*Object)->TryGetNumberField(TEXT("enhancement"), IntValue))
		{
			Entry.EnhancementLevel = IntValue;
		}
		if ((*Object)->TryGetStringField(TEXT("grade"), StringValue) && !StringValue.IsEmpty())
		{
			Entry.GradeTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*StringValue), false);
		}
		if ((*Object)->TryGetNumberField(TEXT("deviation"), DoubleValue))
		{
			Entry.StatDeviation = static_cast<float>(DoubleValue);
		}
		if ((*Object)->TryGetNumberField(TEXT("seed"), IntValue))
		{
			Entry.RandomSeed = IntValue;
		}

		// 인첸트 롤 복원
		const TArray<TSharedPtr<FJsonValue>>* Options = nullptr;
		if ((*Object)->TryGetArrayField(TEXT("rolledOptions"), Options) && Options != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& OptionValue : *Options)
			{
				const TSharedPtr<FJsonObject>* OptionObject = nullptr;
				if (!OptionValue->TryGetObject(OptionObject) || OptionObject == nullptr) continue;

				FRolledEnchantOption Option;
				if ((*OptionObject)->TryGetStringField(TEXT("optionId"), StringValue))
				{
					Option.OptionId = FName(*StringValue);
				}

				const TArray<TSharedPtr<FJsonValue>>* Magnitudes = nullptr;
				if ((*OptionObject)->TryGetArrayField(TEXT("magnitudes"), Magnitudes) && Magnitudes != nullptr)
				{
					for (const TSharedPtr<FJsonValue>& MagnitudeValue : *Magnitudes)
					{
						const TSharedPtr<FJsonObject>* MagnitudeObject = nullptr;
						if (!MagnitudeValue->TryGetObject(MagnitudeObject) || MagnitudeObject == nullptr) continue;

						FRolledMagnitude Magnitude;
						if ((*MagnitudeObject)->TryGetStringField(TEXT("tag"), StringValue) && !StringValue.IsEmpty())
						{
							Magnitude.MagnitudeTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*StringValue), false);
						}
						if ((*MagnitudeObject)->TryGetNumberField(TEXT("value"), DoubleValue))
						{
							Magnitude.Value = static_cast<float>(DoubleValue);
						}
						Option.Magnitudes.Add(Magnitude);
					}
				}
				Entry.RolledOptions.Add(Option);
			}
		}

		// 소켓 젬 복원
		const TArray<TSharedPtr<FJsonValue>>* Sockets = nullptr;
		if ((*Object)->TryGetArrayField(TEXT("sockets"), Sockets) && Sockets != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& GemValue : *Sockets)
			{
				FGuid GemId;
				if (FGuid::Parse(GemValue->AsString(), GemId))
				{
					Entry.SocketedGemInstanceIds.Add(GemId);
				}
			}
		}

		InsertEntry(Entry);
	}
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


int32 UInventoryComponent::GetCapacity() const
{
	return Capacity;
}

FGameplayTag UInventoryComponent::GetContainerTag() const
{
	return GYGameplayTags::Container_Inventory;
}

int32 UInventoryComponent::GetOccupiedSlotCount() const
{
	int32 EquippedCount = 0;

	const AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwner());
	const UEquipmentLoadoutComponent* Loadout = IsValid(PS) ? PS->GetEquipmentLoadoutComponent() : nullptr;
	if (IsValid(Loadout))
	{
		for (const FEquipmentLoadoutEntry& LoadoutEntry : Loadout->GetEntries())
		{
			// 로드아웃이 가리키는 InstanceId가 실제 가방에 있을 때만 차감 (장착 = 가방 슬롯 점유 해제)
			if (LoadoutEntry.InstanceId.IsValid() && FindEntry(LoadoutEntry.InstanceId) != nullptr)
			{
				++EquippedCount;
			}
		}
	}

	return Inventory.Entries.Num() - EquippedCount;
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

