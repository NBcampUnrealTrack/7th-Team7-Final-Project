#include "Widget/MainMenu/GYFadePanelWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"

void UGYFadePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsOpen = false;
	bClosing = false;
	SetVisibility(ESlateVisibility::Collapsed);

	FadeFinishedDelegate.BindDynamic(this, &UGYFadePanelWidget::HandleFadeFinished);

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &UGYFadePanelWidget::HandleBackClicked);
	}
}

void UGYFadePanelWidget::OpenPanel()
{
	if (bIsOpen) return;
	bIsOpen = true;
	bClosing = false;

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (FadeAnim)
	{
		UnbindAllFromAnimationFinished(FadeAnim);
		PlayAnimationForward(FadeAnim);
	}
}

void UGYFadePanelWidget::ClosePanel()
{
	if (!bIsOpen) return;
	bIsOpen = false;
	bClosing = true;

	if (FadeAnim)
	{
		UnbindAllFromAnimationFinished(FadeAnim);
		BindToAnimationFinished(FadeAnim, FadeFinishedDelegate);
		PlayAnimationReverse(FadeAnim);
	}
	else
	{
		HandleFadeFinished();
	}
}

void UGYFadePanelWidget::HandleBackClicked()
{
	ClosePanel();
}

void UGYFadePanelWidget::HandleFadeFinished()
{
	if (!bClosing) return;
	bClosing = false;
	SetVisibility(ESlateVisibility::Collapsed);
	OnPanelClosed.Broadcast();
}
