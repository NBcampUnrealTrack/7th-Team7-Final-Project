#include "Widget/Character/GYCharacterScreenWidget.h"

#include "Equipment/EquipmentLoadoutComponent.h"
#include "Player/GYPlayerState.h"
#include "Widget/Equipment/GYEquipmentPanelWidget.h"
#include "Widget/Inventory/GYInventoryScreenWidget.h"

void UGYCharacterScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InventoryScreen = Cast<UGYInventoryScreenWidget>(GetWidgetFromName(TEXT("WBP_InventoryScreen")));
	if (InventoryScreen)
	{
		InventoryScreen->OnItemClicked.AddDynamic(this, &UGYCharacterScreenWidget::HandleInventoryItemClicked);
	}

	EquipmentPanel = Cast<UGYEquipmentPanelWidget>(GetWidgetFromName(TEXT("WBP_EquipmentPanel")));
	if (EquipmentPanel)
	{
		EquipmentPanel->OnSlotClicked.AddDynamic(this, &UGYCharacterScreenWidget::HandleEquipSlotClicked);
	}
}

void UGYCharacterScreenWidget::NativeDestruct()
{
	if (InventoryScreen)
	{
		InventoryScreen->OnItemClicked.RemoveDynamic(this, &UGYCharacterScreenWidget::HandleInventoryItemClicked);
	}
	if (EquipmentPanel)
	{
		EquipmentPanel->OnSlotClicked.RemoveDynamic(this, &UGYCharacterScreenWidget::HandleEquipSlotClicked);
	}

	Super::NativeDestruct();
}

void UGYCharacterScreenWidget::HandleInventoryItemClicked(FGuid InstanceId)
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UEquipmentLoadoutComponent* Loadout = IsValid(PS) ? PS->GetEquipmentLoadoutComponent() : nullptr;
	if (IsValid(Loadout))
	{
		Loadout->Server_RequestEquip(InstanceId);
	}
}

void UGYCharacterScreenWidget::HandleEquipSlotClicked(FGameplayTag SlotTag, FGuid InstanceId)
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UEquipmentLoadoutComponent* Loadout = IsValid(PS) ? PS->GetEquipmentLoadoutComponent() : nullptr;
	if (IsValid(Loadout))
	{
		Loadout->Server_RequestUnequip(SlotTag);
	}
}
