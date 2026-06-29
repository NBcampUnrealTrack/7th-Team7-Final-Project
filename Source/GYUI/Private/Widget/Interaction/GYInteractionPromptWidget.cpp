#include "Widget/Interaction/GYInteractionPromptWidget.h"
#include "CommonActivatableWidget.h"
#include "CommonTextBlock.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GYPrimaryGameLayout.h"
#include "Core/GYUIManagerSubsystem.h"
#include "GameplayTags/GYUILayerTags.h"
#include "UI/GYUIMessages.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UGYInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);

	ListenForMessage<FGYInteractionOptionsMessage>(GYGameplayTags::Message_Interaction_OptionsChanged,
		[this](FGameplayTag, const FGYInteractionOptionsMessage& Message)
		{
			HandleOptionsChanged(Message.Options);
		});
	BindOverlayLayers();
}

void UGYInteractionPromptWidget::NativeDestruct()
{
    for (const TWeakObjectPtr<UCommonActivatableWidgetContainerBase>& Layer : WatchedLayers)
    {
        if (Layer.IsValid())
        {
            Layer->OnDisplayedWidgetChanged().RemoveAll(this);
        }
    }
    WatchedLayers.Reset();

    Super::NativeDestruct();
}

void UGYInteractionPromptWidget::BindOverlayLayers()
{
    const ULocalPlayer* LP = GetOwningLocalPlayer();
    if (!LP) return;

    UGYUIManagerSubsystem* UI = LP->GetSubsystem<UGYUIManagerSubsystem>();
    if (!UI) return;

    UGYPrimaryGameLayout* Layout = UI->GetPrimaryGameLayout();
    if (!Layout) return;

    auto Bind = [this, Layout](FGameplayTag LayerTag)
    {
        if (UCommonActivatableWidgetContainerBase* Layer = Layout->GetLayerWidget(LayerTag))
        {
            Layer->OnDisplayedWidgetChanged().AddUObject(this, &UGYInteractionPromptWidget::HandleDisplayedWidgetChanged);
            WatchedLayers.Add(Layer);
        }
    };
    Bind(GYUILayerTags::UI_Layer_Menu);
    Bind(GYUILayerTags::UI_Layer_Modal);
}

void UGYInteractionPromptWidget::HandleDisplayedWidgetChanged(UCommonActivatableWidget* /*DisplayedWidget*/)
{
    RefreshVisibility();
}

bool UGYInteractionPromptWidget::IsOverlayActive() const
{
    for (const TWeakObjectPtr<UCommonActivatableWidgetContainerBase>& Layer : WatchedLayers)
    {
        if (Layer.IsValid() && Layer->GetActiveWidget() != nullptr)
        {
            return true;
        }
    }
    return false;
}

void UGYInteractionPromptWidget::HandleOptionsChanged(const TArray<FInteractionOption>& Options)
{
    LastOptions = Options;
    RefreshVisibility();
}

void UGYInteractionPromptWidget::RefreshVisibility()
{
    if (LastOptions.Num() == 0 || IsOverlayActive())
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const FInteractionOption* Best = &LastOptions[0];
    for (const FInteractionOption& Option : LastOptions)
    {
        if (Option.Priority > Best->Priority) Best = &Option;
    }

	if (PromptText) PromptText->SetText(Best->Text);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
