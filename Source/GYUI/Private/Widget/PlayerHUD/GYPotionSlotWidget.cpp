#include "Widget/PlayerHUD/GYPotionSlotWidget.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

void UGYPotionSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	DisplayEmpty();

	if (!ChargePoolTag.IsValid()) return;

	ListenForMessage<UGYPotionSlotWidget, FGYPotionSlotMessage>(GYGameplayTags::Message_UI_PotionSlot, this,
	                                                            &UGYPotionSlotWidget::HandlePotionMessage);
}

void UGYPotionSlotWidget::HandlePotionMessage(FGameplayTag, const FGYPotionSlotMessage& Message)
{
	if (!Message.ChargePoolTag.MatchesTagExact(ChargePoolTag)) return;

	if (Message.bIsEmpty)
	{
		DisplayEmpty();
		OnPotionSlotUpdated(0, true);
		return;
	}

	if (Image_Icon)
	{
		Image_Icon->SetOpacity(1.f);
		// 이미지 로드
		if (UTexture2D* Texture = Message.Icon.LoadSynchronous())
		{
			Image_Icon->SetBrushFromTexture(Texture);
		}
	}

	if (Text_Count)
	{
		Text_Count->SetText(FText::AsNumber(Message.StackCount));
	}

	OnPotionSlotUpdated(Message.StackCount, false);
}

void UGYPotionSlotWidget::DisplayEmpty()
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
			// 빈 솔롯 이미지 따로 없으면 투명도 30%
			Image_Icon->SetOpacity(0.3f);
		}
	}

	if (Text_Count)
	{
		Text_Count->SetText(FText::GetEmpty());
	}
}
