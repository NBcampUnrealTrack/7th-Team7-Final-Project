#include "Widget/Interact/GYAltarWidget.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Core/GYItemDragDropOperation.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/AltarStorageComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Inventory/ItemTransactionComponent.h"
#include "Items/ItemContainer.h"
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

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UGYAltarWidget::OnCloseButtonClicked);
	}

	if (ExecuteButton)
	{
		ExecuteButton->OnClicked.AddDynamic(this, &UGYAltarWidget::OnExecuteButtonClicked);
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
	CloseButton->OnClicked.RemoveDynamic(this, &UGYAltarWidget::OnCloseButtonClicked);
	ExecuteButton->OnClicked.RemoveDynamic(this, &UGYAltarWidget::OnExecuteButtonClicked);
	Super::NativeDestruct();
}

bool UGYAltarWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                  UDragDropOperation* InOperation)
{
	UGYItemDragDropOperation* DragOperation = Cast<UGYItemDragDropOperation>(InOperation);
	if (!DragOperation || !DragOperation->FromContainer) return false;
	if (!Container) return false;

	if (DragOperation->OriginSlotWidget.IsValid())
	{
		DragOperation->OriginSlotWidget->SetRenderOpacity(1.0f);
	}

	// 제단은 분해 제물 전용 — 장비만 올릴 수 있음. 비장비 드롭은 서버 전송 전에 거부.
	const FInventoryEntry* DraggedEntry = DragOperation->FromContainer->FindEntry(DragOperation->FromInstanceId);
	if (DraggedEntry == nullptr) return false;
	const UItemDefinition* DraggedDef = DraggedEntry->Definition.LoadSynchronous();
	if (!IsValid(DraggedDef) || !DraggedDef->CategoryTags.HasTag(GYGameplayTags::Item_Category_Equipment))
	{
		return false;
	}

	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		if (UItemTransactionComponent* Transaction = PS->GetItemTransactionComponent())
		{
			Transaction->Server_TransferItem(
				DragOperation->FromContainer->GetContainerTag(),
				DragOperation->FromInstanceId,
				Container->GetContainerTag());
		}
	}

	return true;
}

void UGYAltarWidget::OnCloseButtonClicked()
{
	RequestExit();
}

void UGYAltarWidget::OnExecuteButtonClicked()
{
	UAltarStorageComponent* AltarStorageComponent = ResolveAltarStorage();
	if (!AltarStorageComponent) return;

	AltarStorageComponent->Server_RequestDisassemble();



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

FGameplayTag UGYAltarWidget::GetExitEventTag() const
{
	return GYGameplayTags::Event_TimeRift_Altar_Exit;
}
