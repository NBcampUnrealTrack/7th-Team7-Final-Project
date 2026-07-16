#include "Widget/MainMenu/GYSlidePanelWidget.h"
#include "Animation/WidgetAnimation.h"

void UGYSlidePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 시작 상태 - 닫힘
	bIsOpen = false;
	bClosing = false;
	SetVisibility(ESlateVisibility::Collapsed);

	SlideFinishedDelegate.BindDynamic(this, &UGYSlidePanelWidget::HandleSlideFinished);
}

void UGYSlidePanelWidget::OpenPanel()
{
	if (bIsOpen) return;
	bIsOpen = true;
	bClosing = false;

	// 자식 버튼은 클릭 가능하게
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (SlideAnim)
	{
		UnbindAllFromAnimationFinished(SlideAnim); // 이전 닫힘 콜백 제거
		PlayAnimationForward(SlideAnim);
	}
}

void UGYSlidePanelWidget::ClosePanel()
{
	if (!bIsOpen) return;
	bIsOpen = false;
	bClosing = true;

	if (SlideAnim)
	{
		UnbindAllFromAnimationFinished(SlideAnim);
		BindToAnimationFinished(SlideAnim, SlideFinishedDelegate);
		PlayAnimationReverse(SlideAnim);
	}
	else
	{
		HandleSlideFinished(); // 애니메이션 없으면 즉시 처리
	}
}

void UGYSlidePanelWidget::TogglePanel()
{
	bIsOpen ? ClosePanel() : OpenPanel();
}

void UGYSlidePanelWidget::HandleSlideFinished()
{
	if (!bClosing) return; // 열림 애니 끝은 무시, 닫힘만 처리
	bClosing = false;
	SetVisibility(ESlateVisibility::Collapsed);
	OnPanelClosed.Broadcast();
}
