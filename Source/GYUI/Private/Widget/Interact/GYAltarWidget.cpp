// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Interact/GYAltarWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Core/GYItemDragDropOperation.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/AltarStorageComponent.h"
#include "Inventory/GA_TransferItem.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

#include "Widget/Inventory/GYItemSlotWidget.h"

void UGYAltarWidget::NativeConstruct()
{
	Super::NativeConstruct();


	UAltarStorageComponent* AltarStorageComponent = ResolveAltarStorage();
	if (AltarStorageComponent)
	{
		Container = AltarStorageComponent;
		GridSlotCount = AltarStorageComponent->GetCapacity();
	}
	EnsureSlots();

	if (UWorld* World = GetWorld())
	{
		ListenerHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(
			GYGameplayTags::Message_Altar_EntryChanged,
			this,
			&UGYAltarWidget::HandleEntryChanged);
	}

	Refresh();
}

void UGYAltarWidget::NativeDestruct()
{
	if (ListenerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayMessageSubsystem::Get(World).UnregisterListener(ListenerHandle);
		}
		ListenerHandle = FGameplayMessageListenerHandle();
	}
	Super::NativeDestruct();
}

bool UGYAltarWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                  UDragDropOperation* InOperation)
{
	UGYItemDragDropOperation* DragOperation = Cast<UGYItemDragDropOperation>(InOperation);
	if (!DragOperation || !DragOperation->FromContainer) return false;
	if (!Container) return false;

	DragOperation->OriginSlotWidget->SetRenderOpacity(1.0f);

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

void UGYAltarWidget::OnCloseButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;

	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_Altar_Exit, FGameplayEventData());
}

void UGYAltarWidget::EnsureSlots()
{
	if (SlotWidgets.Num() == GridSlotCount) return;
	if (SlotContainer == nullptr || SlotWidgetClass == nullptr) return;

	SlotContainer->ClearChildren();
	SlotWidgets.Reset(GridSlotCount);
	UAltarStorageComponent* AltarStorage = ResolveAltarStorage();
	for (int32 i = 0; i < GridSlotCount; ++i)
	{
		UGYItemSlotWidget* SlotWidget = WidgetTree->ConstructWidget<UGYItemSlotWidget>(SlotWidgetClass);
		if (SlotWidget == nullptr) continue;
		SlotWidget->SetContainer(AltarStorage);
		if (SlotWidget == nullptr) continue;

		SlotContainer->AddChild(SlotWidget);
		SlotWidgets.Add(SlotWidget);
	}


}


void UGYAltarWidget::Refresh()
{
	UAltarStorageComponent* AltarStorage = ResolveAltarStorage();
	Container = AltarStorage;
	if (AltarStorage == nullptr)
	{
		for (UGYItemSlotWidget* SlotWidget : SlotWidgets)
		{
			if (IsValid(SlotWidget)) SlotWidget->SetEmpty();
		}
		return;
	}

	const TArray<FInventoryEntry>& Entries = AltarStorage->GetEntries();

	int32 SlotIndex = 0;

	for (const FInventoryEntry& Entry : Entries)
	{
		if (SlotIndex >= SlotWidgets.Num()) break;

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

UAltarStorageComponent* UGYAltarWidget::ResolveAltarStorage() const
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	return IsValid(PS) ? PS->GetAltarStorageComponent() : nullptr;
}

void UGYAltarWidget::HandleEntryChanged(FGameplayTag, const FGYInventoryEntryMessage&)
{
	Refresh();
}
