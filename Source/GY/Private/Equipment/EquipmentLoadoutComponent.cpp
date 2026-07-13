#include "Equipment/EquipmentLoadoutComponent.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GameplayTagsManager.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/SoundTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/Fragments/ItemFragment_Equippable.h"
#include "Items/ItemDefinition.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

UEquipmentLoadoutComponent::UEquipmentLoadoutComponent()
{
	SetIsReplicatedByDefault(true);
}

void UEquipmentLoadoutComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UEquipmentLoadoutComponent, LoadoutEntries, Params);
}

void UEquipmentLoadoutComponent::Server_RequestEquip_Implementation(const FGuid& InstanceId)
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwner());
	if (!IsValid(PS)) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!IsValid(Inv)) return;

	const FInventoryEntry* Entry = Inv->FindEntry(InstanceId);
	if (Entry == nullptr) return;

	UItemDefinition* Def = Entry->Definition.LoadSynchronous();
	if (!IsValid(Def)) return;

	const UItemFragment_Equippable* EquippableFragment = Def->FindFragment<UItemFragment_Equippable>();
	if (EquippableFragment == nullptr) return;

	if (SetSlot(EquippableFragment->SlotTag, InstanceId))
	{
		PS->Client_PlaySound(GYGameplayTags::Sound_Item_Looting);
	}
}

void UEquipmentLoadoutComponent::Server_RequestUnequip_Implementation(FGameplayTag SlotTag)
{
	// 가방이 꽉 차 있으면 해제 불가 — 해제하면 아이템이 다시 가방 슬롯을 차지하는데 들어갈 칸이 없음
	// (장착 아이템은 점유 카운트에서 제외돼 있으므로, 점유 >= 용량이면 해제 시 초과)
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwner());
	UInventoryComponent* Inv = IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
	if (IsValid(Inv) && Inv->GetOccupiedSlotCount() >= Inv->GetCapacity()) return;

	ClearSlot(SlotTag);
}

bool UEquipmentLoadoutComponent::SetSlot(FGameplayTag SlotTag, const FGuid& InstanceId)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!SlotTag.IsValid()) return false;
	if (!InstanceId.IsValid()) return false;

	const int32 Index = LoadoutEntries.IndexOfByPredicate([&SlotTag](const FEquipmentLoadoutEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	if (Index == INDEX_NONE)
	{
		FEquipmentLoadoutEntry NewEntry;
		NewEntry.SlotTag = SlotTag;
		NewEntry.InstanceId = InstanceId;
		LoadoutEntries.Add(NewEntry);
	}
	else
	{
		if (LoadoutEntries[Index].InstanceId == InstanceId) return false;
		LoadoutEntries[Index].InstanceId = InstanceId;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UEquipmentLoadoutComponent, LoadoutEntries, this);
	BroadcastSlotChanged(SlotTag, InstanceId);

	return true;
}

TSharedPtr<FJsonValue> UEquipmentLoadoutComponent::ExportSaveData() const
{
	// { "Equipment.Slot.Weapon": "guid", ... } — 슬롯 태그 → 장착 아이템 InstanceId
	const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
	for (const FEquipmentLoadoutEntry& Entry : LoadoutEntries)
	{
		if (Entry.SlotTag.IsValid() && Entry.InstanceId.IsValid())
		{
			Object->SetStringField(Entry.SlotTag.ToString(), Entry.InstanceId.ToString(EGuidFormats::Digits));
		}
	}
	return MakeShared<FJsonValueObject>(Object);
}

void UEquipmentLoadoutComponent::ImportSaveData(const TSharedPtr<FJsonValue>& Data)
{
	// 인벤 InstanceId 를 참조하므로 인벤 복원 이후에 호출돼야 함 (ApplySaveData 가 순서 보장).
	if (!GetOwner()->HasAuthority()) return;
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* Object = nullptr;
	if (!Data->TryGetObject(Object) || Object == nullptr) return;

	// 기존 슬롯 비우기 (각 슬롯 정리 → 외형/어빌리티 해제 트리거)
	TArray<FGameplayTag> ExistingSlots;
	for (const FEquipmentLoadoutEntry& Entry : LoadoutEntries)
	{
		ExistingSlots.Add(Entry.SlotTag);
	}
	for (const FGameplayTag& SlotTag : ExistingSlots)
	{
		ClearSlot(SlotTag);
	}

	// 세이브의 슬롯 → InstanceId 로 장착 복원
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Object)->Values)
	{
		const FGameplayTag SlotTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*Pair.Key), false);
		if (!SlotTag.IsValid()) continue;

		FGuid InstanceId;
		if (FGuid::Parse(Pair.Value->AsString(), InstanceId))
		{
			SetSlot(SlotTag, InstanceId);
		}
	}
}

bool UEquipmentLoadoutComponent::ClearSlot(FGameplayTag SlotTag)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!SlotTag.IsValid()) return false;

	const int32 Index = LoadoutEntries.IndexOfByPredicate([&SlotTag](const FEquipmentLoadoutEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	if (Index == INDEX_NONE) return false;

	LoadoutEntries.RemoveAt(Index);

	MARK_PROPERTY_DIRTY_FROM_NAME(UEquipmentLoadoutComponent, LoadoutEntries, this);
	BroadcastSlotChanged(SlotTag, FGuid());

	return true;
}

bool UEquipmentLoadoutComponent::GetSlot(FGameplayTag SlotTag, FGuid& OutInstanceId) const
{
	const FEquipmentLoadoutEntry* Found = LoadoutEntries.FindByPredicate([&SlotTag](const FEquipmentLoadoutEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});

	if (Found == nullptr) return false;

	OutInstanceId = Found->InstanceId;
	return true;
}

void UEquipmentLoadoutComponent::BroadcastSlotChanged(FGameplayTag SlotTag, const FGuid& InstanceId)
{
	OnLoadoutSlotChanged.Broadcast(SlotTag, InstanceId);

	UWorld* World = GetWorld();
	if (World == nullptr || World->IsNetMode(NM_DedicatedServer)) return;

	FGYEquipSlotMessage Msg;
	Msg.SlotTag = SlotTag;
	Msg.bIsEmpty = true;

	if (InstanceId.IsValid())
	{
		AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwner());
		UInventoryComponent* Inv = IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
		const FInventoryEntry* Entry = IsValid(Inv) ? Inv->FindEntry(InstanceId) : nullptr;

		if (Entry != nullptr)
		{
			if (UItemDefinition* Def = Entry->Definition.LoadSynchronous())
			{
				Msg.Icon = Def->Icon;
				Msg.bIsEmpty = false;
			}
		}
	}

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Equipment_LoadoutSlotChanged, Msg);
}

void UEquipmentLoadoutComponent::OnRep_LoadoutEntries(const TArray<FEquipmentLoadoutEntry>& OldEntries)
{
	for (const FEquipmentLoadoutEntry& OldEntry : OldEntries)
	{
		const FEquipmentLoadoutEntry* NewEntry = LoadoutEntries.FindByPredicate([&OldEntry](const FEquipmentLoadoutEntry& Entry)
		{
			return Entry.SlotTag == OldEntry.SlotTag;
		});

		if (NewEntry == nullptr)
		{
			BroadcastSlotChanged(OldEntry.SlotTag, FGuid());
		}
		else if (NewEntry->InstanceId != OldEntry.InstanceId)
		{
			BroadcastSlotChanged(NewEntry->SlotTag, NewEntry->InstanceId);
		}
	}

	for (const FEquipmentLoadoutEntry& NewEntry : LoadoutEntries)
	{
		const bool bWasInOld = OldEntries.ContainsByPredicate([&NewEntry](const FEquipmentLoadoutEntry& Entry)
		{
			return Entry.SlotTag == NewEntry.SlotTag;
		});

		if (!bWasInOld)
		{
			BroadcastSlotChanged(NewEntry.SlotTag, NewEntry.InstanceId);
		}
	}
}
