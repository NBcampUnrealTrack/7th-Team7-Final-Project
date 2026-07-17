#include "Widget/Interact/GYAltarWidget.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Core/GYItemDragDropOperation.h"
#include "Core/GameplayTags/CurrencyTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Currency/CurrencyComponent.h"
#include "Enchant/EnchantCostRow.h"
#include "Enchant/GYEnchantSettings.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/AltarStorageComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Inventory/ItemTransactionComponent.h"
#include "Items/ItemContainer.h"
#include "Items/ItemDefinition.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

#include "Widget/Inventory/GYInventoryScreenWidget.h"
#include "Widget/Inventory/GYItemSlotWidget.h"

#define LOCTEXT_NAMESPACE "GYUI"

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

	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		if (UCurrencyComponent* Currency = PS->GetCurrencyComponent())
		{
			const int32 Cur = Currency->GetAmount(GYGameplayTags::Currency_TimeShard);
			OnCurrencyChangedHandle = Currency->OnCurrencyChanged.AddUObject(this, &UGYAltarWidget::OnCurrencyChanged);
			UpdateTimeShardBar(Cur);
		}
	}

	// 임베드된 인벤토리 아이템 좌클릭, 제단으로 이동
	InventoryScreen = Cast<UGYInventoryScreenWidget>(GetWidgetFromName(TEXT("WBP_InventoryScreen")));
	if (InventoryScreen)
	{
		InventoryScreen->OnItemClicked.AddDynamic(this, &UGYAltarWidget::HandleInventoryItemClicked);
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

	if (OnCurrencyChangedHandle.IsValid())
	{
		if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
		{
			if (UCurrencyComponent* Currency = PS->GetCurrencyComponent())
			{
				Currency->OnCurrencyChanged.Remove(OnCurrencyChangedHandle);
			}
		}
		OnCurrencyChangedHandle.Reset();
	}

	if (InventoryScreen)
	{
		InventoryScreen->OnItemClicked.RemoveDynamic(this, &UGYAltarWidget::HandleInventoryItemClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UGYAltarWidget::OnCloseButtonClicked);
	}
	if (ExecuteButton)
	{
		ExecuteButton->OnClicked.RemoveDynamic(this, &UGYAltarWidget::OnExecuteButtonClicked);
	}
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

		// 제단 슬롯 좌클릭 -> 인벤으로 자동 이동
		SlotWidget->OnSlotClicked.AddUObject(this, &UGYAltarWidget::HandleAltarSlotClicked);

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

void UGYAltarWidget::OnCurrencyChanged(FGameplayTag CurrencyTag, int32 Amount)
{
	if (!CurrencyTag.MatchesTagExact(GYGameplayTags::Currency_TimeShard)) return;
	UpdateTimeShardBar(Amount);
}

int32 UGYAltarWidget::GetTimeShardMax() const
{
	const UGYEnchantSettings* Settings = GetDefault<UGYEnchantSettings>();
	if (UDataTable* CostTable = IsValid(Settings) ? Settings->EnchantCostTable.LoadSynchronous() : nullptr)
	{
		if (const FEnchantCostRow* Row =
			CostTable->FindRow<FEnchantCostRow>(Settings->DefaultCostRowName, TEXT("GYAltarWidget")))
		{
			return FMath::Max(1, Row->Amount);
		}
	}
	return 100;
}

void UGYAltarWidget::UpdateTimeShardBar(int32 Amount)
{
	const int32 Max = GetTimeShardMax();
	if (ProgressBar)
	{
		ProgressBar->SetPercent(FMath::Clamp(static_cast<float>(Amount) / Max, 0.f, 1.f));
	}
	if (Text_TimeShard)
	{
		Text_TimeShard->SetText(FText::Format(LOCTEXT("Altar_TimeShardFormat", "{0} / {1}"),
			FText::AsNumber(Amount), FText::AsNumber(Max)));
	}
}

void UGYAltarWidget::HandleInventoryItemClicked(FGuid InstanceId)
{
	// 인벤 -> 제단, 제단은 장비만 받으므로 비장비 클릭은 무시
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UInventoryComponent* Inv = IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
	const FInventoryEntry* Entry = IsValid(Inv) ? Inv->FindEntry(InstanceId) : nullptr;
	if (Entry == nullptr) return;

	const UItemDefinition* Def = Entry->Definition.LoadSynchronous();
	if (!IsValid(Def) || !Def->CategoryTags.HasTag(GYGameplayTags::Item_Category_Equipment)) return;

	TransferItem(GYGameplayTags::Container_Inventory, InstanceId, GYGameplayTags::Container_Altar);
}

void UGYAltarWidget::HandleAltarSlotClicked(const FGuid& InstanceId)
{
	// 제단 -> 인벤
	TransferItem(GYGameplayTags::Container_Altar, InstanceId, GYGameplayTags::Container_Inventory);
}

void UGYAltarWidget::TransferItem(FGameplayTag FromTag, const FGuid& InstanceId, FGameplayTag ToTag)
{
	if (!InstanceId.IsValid()) return;
	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		if (UItemTransactionComponent* Transaction = PS->GetItemTransactionComponent())
		{
			Transaction->Server_TransferItem(FromTag, InstanceId, ToTag);
		}
	}
}

#undef LOCTEXT_NAMESPACE
