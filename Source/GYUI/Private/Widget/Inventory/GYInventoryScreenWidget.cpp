#include "Widget/Inventory/GYInventoryScreenWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Core/GYItemDragDropOperation.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/GA_TransferItem.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"
#include "Widget/Inventory/GYItemSlotWidget.h"

void UGYInventoryScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentCategory = InitialCategory;

	UInventoryComponent* Inv = ResolveInventory();
	Container = Inv;

	EnsureSlots();

	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& Messaging = UGameplayMessageSubsystem::Get(World);
		ListenerHandle = Messaging.RegisterListener(
			GYGameplayTags::Message_Inventory_EntryChanged,
			this,
			&UGYInventoryScreenWidget::HandleEntryChanged);
		LoadoutListenerHandle = Messaging.RegisterListener(
			GYGameplayTags::Message_Equipment_LoadoutSlotChanged,
			this,
			&UGYInventoryScreenWidget::HandleLoadoutChanged);
	}

	Refresh();
	OnCategoryChanged(CurrentCategory);
}

void UGYInventoryScreenWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& Messaging = UGameplayMessageSubsystem::Get(World);
		if (ListenerHandle.IsValid()) Messaging.UnregisterListener(ListenerHandle);
		if (LoadoutListenerHandle.IsValid()) Messaging.UnregisterListener(LoadoutListenerHandle);
	}
	ListenerHandle = FGameplayMessageListenerHandle();
	LoadoutListenerHandle = FGameplayMessageListenerHandle();

	Super::NativeDestruct();
}

bool UGYInventoryScreenWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UGYItemDragDropOperation* DragOperation = Cast<UGYItemDragDropOperation>(InOperation);
	if (!DragOperation || !DragOperation->FromContainer) return false;
	if (!Container) return false;

	if (DragOperation->OriginSlotWidget.IsValid())
	{
		DragOperation->OriginSlotWidget->SetRenderOpacity(1.0f);
	}
	if (DragOperation->FromContainer.GetObject() == Container.GetObject())
	{
		return true;
	}

	UItemTransferPayload* Payload = NewObject<UItemTransferPayload>(this);
	Payload->FromContainer = DragOperation->FromContainer;
	Payload->FromInstanceId = DragOperation->FromInstanceId;
	Payload->ToContainer = Container;

	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
		{
			FGameplayEventData EventData;
			EventData.OptionalObject = Payload;
			ASC->Server_SendGameplayEvent(GYGameplayTags::Event_ItemContainer_Transfer, EventData);
		}
	}

	return true;
}

void UGYInventoryScreenWidget::NotifyItemClicked(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid()) return;
	OnItemClicked.Broadcast(InstanceId);
}

void UGYInventoryScreenWidget::SetCategory(FGameplayTag CategoryTag)
{
	if (CurrentCategory == CategoryTag) return;

	CurrentCategory = CategoryTag;
	Refresh();
	OnCategoryChanged(CurrentCategory);
}

void UGYInventoryScreenWidget::EnsureSlots()
{
	if (SlotWidgets.Num() == GridSlotCount) return;
	if (SlotContainer == nullptr || SlotWidgetClass == nullptr) return;

	SlotContainer->ClearChildren();
	SlotWidgets.Reset(GridSlotCount);
	UInventoryComponent* Inv = ResolveInventory();
	for (int32 i = 0; i < GridSlotCount; ++i)
	{
		UGYItemSlotWidget* SlotWidget = WidgetTree->ConstructWidget<UGYItemSlotWidget>(SlotWidgetClass);
		if (SlotWidget == nullptr) continue;
		SlotWidget->SetContainer(Inv);
		if (SlotWidget == nullptr) continue;

		SlotContainer->AddChild(SlotWidget);
		SlotWidgets.Add(SlotWidget);
	}
}

void UGYInventoryScreenWidget::Refresh()
{
	UInventoryComponent* Inv = ResolveInventory();
	Container = Inv;
	if (Inv == nullptr)
	{
		for (UGYItemSlotWidget* SlotWidget : SlotWidgets)
		{
			if (IsValid(SlotWidget)) SlotWidget->SetEmpty();
		}
		return;
	}

	// 장착 중인 아이템은 가방 그리드에서 숨김 (로드아웃이 InstanceId로 참조만 하고 인벤엔 남아있음)
	TSet<FGuid> EquippedIds;
	if (UEquipmentLoadoutComponent* Loadout = ResolveLoadout())
	{
		for (const FEquipmentLoadoutEntry& LoadoutEntry : Loadout->GetEntries())
		{
			if (LoadoutEntry.InstanceId.IsValid())
			{
				EquippedIds.Add(LoadoutEntry.InstanceId);
			}
		}
	}

	const TArray<FInventoryEntry>& Entries = Inv->GetEntries();
	int32 SlotIndex = 0;

	for (const FInventoryEntry& Entry : Entries)
	{
		if (SlotIndex >= SlotWidgets.Num()) break;

		if (EquippedIds.Contains(Entry.InstanceId)) continue;

		if (CurrentCategory.IsValid())
		{
			UItemDefinition* Def = Entry.Definition.LoadSynchronous();
			if (!IsValid(Def) || !Def->CategoryTags.HasTag(CurrentCategory)) continue;
		}

		if (UGYItemSlotWidget* SlotWidget = SlotWidgets[SlotIndex])
		{
			SlotWidget->SetEntry(Entry);
		}
		++SlotIndex;
	}

	for (int32 i = SlotIndex; i < SlotWidgets.Num(); ++i)
	{
		if (UGYItemSlotWidget* SlotWidget = SlotWidgets[i])
		{
			SlotWidget->SetEmpty();
		}
	}
}

UInventoryComponent* UGYInventoryScreenWidget::ResolveInventory() const
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	return IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
}

UEquipmentLoadoutComponent* UGYInventoryScreenWidget::ResolveLoadout() const
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	return IsValid(PS) ? PS->GetEquipmentLoadoutComponent() : nullptr;
}

void UGYInventoryScreenWidget::HandleEntryChanged(FGameplayTag, const FGYInventoryEntryMessage&)
{
	Refresh();
}

void UGYInventoryScreenWidget::HandleLoadoutChanged(FGameplayTag, const FGYEquipSlotMessage&)
{
	Refresh();
}
