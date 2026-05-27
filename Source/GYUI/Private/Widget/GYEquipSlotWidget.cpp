#include "Widget/GYEquipSlotWidget.h"
#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"


void UGYEquipSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	DisplayEmpty();

	if (!SlotTag.IsValid()) return;

	ListenForMessage<UGYEquipSlotWidget, FGYEquipSlotMessage>(GYGameplayTags::Message_UI_EquipmentSlot, this,
	                                                          &UGYEquipSlotWidget::HandleEquipMessage);
}

void UGYEquipSlotWidget::HandleEquipMessage(FGameplayTag Channel, const FGYEquipSlotMessage& Message)
{
	if (!Message.SlotTag.MatchesTagExact(SlotTag)) return;

	if (Message.bIsEmpty)
	{
		DisplayEmpty();
		OnEquipSlotUpdated(true);
		return;
	}

	if (Image_Icon)
	{
		Image_Icon->SetOpacity(1.f);
		if (UTexture2D* Texture = Message.Icon.LoadSynchronous())
		{
			Image_Icon->SetBrushFromTexture(Texture);
		}
	}
	OnEquipSlotUpdated(false);
}

void UGYEquipSlotWidget::DisplayEmpty()
{
	if (Image_Icon)
	{
		if (UTexture2D* Texture = EmptySlotIcon.LoadSynchronous())
		{
			Image_Icon->SetBrushFromTexture(Texture);
			Image_Icon->SetOpacity(1.f);
		}
		else
		{
			Image_Icon->SetOpacity(1.f);
		}
	}
}
