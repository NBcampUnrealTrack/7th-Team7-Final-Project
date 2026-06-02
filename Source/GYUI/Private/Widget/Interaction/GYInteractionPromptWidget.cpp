#include "Widget/Interaction/GYInteractionPromptWidget.h"
#include "CommonTextBlock.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

void UGYInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);

	ListenForMessage<FGYInteractionOptionsMessage>(
		GYGameplayTags::Message_Interaction_OptionsChanged,
		[this](FGameplayTag, const FGYInteractionOptionsMessage& Message)
		{
			HandleOptionsChanged(Message.Options);
		});
}

void UGYInteractionPromptWidget::HandleOptionsChanged(const TArray<FInteractionOption>& Options)
{
	if (Options.Num() == 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FInteractionOption* Best = &Options[0];
	for (const FInteractionOption& Option : Options)
	{
		if (Option.Priority > Best->Priority) Best = &Option;
	}

	if (PromptText) PromptText->SetText(Best->Text);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
