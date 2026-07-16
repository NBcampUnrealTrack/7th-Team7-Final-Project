#include "Widget/MainMenu/GYMainMenuWidget.h"
#include "Widget/MainMenu/GYSessionContainerWidget.h"
#include "Widget/MainMenu/GYSessionCreateWidget.h"
#include "Widget/MainMenu/GYFadePanelWidget.h"
#include "CommonActivatableWidget.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetSystemLibrary.h"

UGYMainMenuWidget::UGYMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EGYWidgetInputMode::Menu;
}

void UGYMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Session) Button_Session->OnClicked.AddDynamic(this, &UGYMainMenuWidget::HandleSessionClicked);
	if (Button_Setting) Button_Setting->OnClicked.AddDynamic(this, &UGYMainMenuWidget::HandleSettingClicked);
	if (Button_Credit) Button_Credit->OnClicked.AddDynamic(this, &UGYMainMenuWidget::HandleCreditClicked);
	if (Button_GameExit) Button_GameExit->OnClicked.AddDynamic(this, &UGYMainMenuWidget::HandleGameExitClicked);

	if (WBP_SessionContainer)
		WBP_SessionContainer->OnCreateSessionRequested.AddDynamic(this, &UGYMainMenuWidget::HandleCreateSessionRequested);

	if (WBP_DevCredit)
		WBP_DevCredit->OnPanelClosed.AddDynamic(this, &UGYMainMenuWidget::HandleCreditClosed);
}

void UGYMainMenuWidget::HandleSessionClicked()
{
	if (WBP_SessionContainer) WBP_SessionContainer->TogglePanel();
}

void UGYMainMenuWidget::HandleCreateSessionRequested()
{
	if (WBP_SessionCreate) WBP_SessionCreate->OpenPanel();
}

void UGYMainMenuWidget::HandleCreditClicked()
{
	if (!WBP_DevCredit) return;
	if (ContentRoot) ContentRoot->SetVisibility(ESlateVisibility::Collapsed); // 밑 레이어 숨김
	WBP_DevCredit->OpenPanel(); // 페이드 인
}

void UGYMainMenuWidget::HandleCreditClosed()
{
	if (ContentRoot) ContentRoot->SetVisibility(ESlateVisibility::Visible); // 복구
}

void UGYMainMenuWidget::HandleSettingClicked()
{
	if (ActiveSettings) return;          // 이미 떠 있으면 무시
	if (!SettingsWidgetClass) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	ActiveSettings = CreateWidget<UCommonActivatableWidget>(PC, SettingsWidgetClass);
	if (!ActiveSettings) return;

	if (ContentRoot) ContentRoot->SetVisibility(ESlateVisibility::Collapsed);

	ActiveSettings->AddToViewport(100);
	ActiveSettings->ActivateWidget();

	// 세팅이 스스로 DeactivateWidget()으로 닫히면 복구
	ActiveSettings->OnDeactivated().AddWeakLambda(this, [this]()
	{
		HandleSettingsClosed();
	});
}

void UGYMainMenuWidget::HandleSettingsClosed()
{
	if (ActiveSettings)
	{
		ActiveSettings->RemoveFromParent();
		ActiveSettings = nullptr;
	}
	if (ContentRoot) ContentRoot->SetVisibility(ESlateVisibility::Visible);
}

void UGYMainMenuWidget::HandleGameExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
