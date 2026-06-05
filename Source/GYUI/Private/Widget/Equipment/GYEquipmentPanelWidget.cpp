#include "Widget/Equipment/GYEquipmentPanelWidget.h"

void UGYEquipmentPanelWidget::NotifySlotClicked(FGameplayTag SlotTag, const FGuid& InstanceId)
{
	if (!SlotTag.IsValid()) return;
	OnSlotClicked.Broadcast(SlotTag, InstanceId);
}
