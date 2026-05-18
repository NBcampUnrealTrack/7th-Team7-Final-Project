#include "Core/GYActivatableWidget.h"
#include "CommonInputTypeEnum.h"

UGYActivatableWidget::UGYActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TOptional<FUIInputConfig> UGYActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputMode)
	{
	case EGYWidgetInputMode::Game: // 게임 모드 - 조작o
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);
	case EGYWidgetInputMode::Menu: // 메뉴 모드 - 조작x, UI만 조작
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
	case EGYWidgetInputMode::Default:
	default:
		return TOptional<FUIInputConfig>();
	}
}
