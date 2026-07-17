#include "Widget/Common/GYMessagePopupWidget.h"
#include "Blueprint/UserWidget.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Core/GYUISettings.h"
#include "Core/GYUIManagerSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameplayTags/GYUILayerTags.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/GYMenuGameMode.h"

UGYMessagePopupWidget::UGYMessagePopupWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EGYWidgetInputMode::Menu; // 커서/버튼 입력 확보
}

void UGYMessagePopupWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 버튼 바인딩은 인스턴스당 1회만 실행
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &UGYMessagePopupWidget::HandleCloseClicked);
	if (Button_Confirm) Button_Confirm->OnClicked.AddDynamic(this, &UGYMessagePopupWidget::HandleConfirmClicked);
	if (Button_Cancel) Button_Cancel->OnClicked.AddDynamic(this, &UGYMessagePopupWidget::HandleCancelClicked);
}

void UGYMessagePopupWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

UWidget* UGYMessagePopupWidget::NativeGetDesiredFocusTarget() const
{
	if (Button_Confirm) return Button_Confirm;
	if (Button_Close)   return Button_Close;
	return const_cast<UGYMessagePopupWidget*>(this);
}

void UGYMessagePopupWidget::SetupNotice(const FText& InTitle, const FText& InMessage)
{
	if (Text_Title) Text_Title->SetText(InTitle);
	if (Text_Message) Text_Message->SetText(InMessage);

	if (Button_Close) Button_Close->SetVisibility(ESlateVisibility::Visible);
	if (Button_Confirm) Button_Confirm->SetVisibility(ESlateVisibility::Collapsed);
	if (Button_Cancel) Button_Cancel->SetVisibility(ESlateVisibility::Collapsed);
}

void UGYMessagePopupWidget::SetupConfirm(const FText& InTitle, const FText& InMessage)
{
	if (Text_Title) Text_Title->SetText(InTitle);
	if (Text_Message) Text_Message->SetText(InMessage);

	if (Button_Close) Button_Close->SetVisibility(ESlateVisibility::Collapsed);
	if (Button_Confirm) Button_Confirm->SetVisibility(ESlateVisibility::Visible);
	if (Button_Cancel) Button_Cancel->SetVisibility(ESlateVisibility::Visible);
}

void UGYMessagePopupWidget::HandleCloseClicked()
{
	OnClosed.Broadcast();    CloseSelf();
}

void UGYMessagePopupWidget::HandleConfirmClicked()
{
	OnConfirmed.Broadcast(); CloseSelf();
}

void UGYMessagePopupWidget::HandleCancelClicked()
{
	OnCancelled.Broadcast(); CloseSelf();
}

void UGYMessagePopupWidget::CloseSelf()
{
	DeactivateWidget();  // Modal 레이어 스택에 있으면 pop
	RemoveFromParent();  // 뷰포트에 직접 추가된 경우 대비
}

UGYMessagePopupWidget* UGYMessagePopupWidget::ShowNotice(const UObject* WC, const FText& T, const FText& M)
{ return ShowInternal(WC, T, M, false); }

UGYMessagePopupWidget* UGYMessagePopupWidget::ShowConfirm(const UObject* WC, const FText& T, const FText& M)
{ return ShowInternal(WC, T, M, true); }

UGYMessagePopupWidget* UGYMessagePopupWidget::ShowInternal(const UObject* WC, const FText& Title, const FText& Message, bool bConfirm)
{
	if (!GEngine || !WC) return nullptr;
	UWorld* World = GEngine->GetWorldFromContextObject(WC, EGetWorldErrorMode::ReturnNull);
	if (!World) return nullptr;

	const UGYUISettings* UISettings = GetDefault<UGYUISettings>();
	TSubclassOf<UGYMessagePopupWidget> PopupClass = UISettings ? UISettings->MessagePopupClass.LoadSynchronous() : nullptr;
	if (!PopupClass) return nullptr;

	APlayerController* PC = World->GetFirstPlayerController();
	UGYMessagePopupWidget* Popup = nullptr;

	const bool bMenuContext =
		(World->GetAuthGameMode<AGYMenuGameMode>() != nullptr) ||World->GetMapName().Contains(TEXT("MainMenu"));

	// 인게임에서만 PrimaryGameLayout - Modal 레이어로 push
	if (!bMenuContext)
	{
		if (ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr)
		{
			if (UGYUIManagerSubsystem* UIM = LP->GetSubsystem<UGYUIManagerSubsystem>())
			{
				if (UIM->GetPrimaryGameLayout())
				{
					Popup = Cast<UGYMessagePopupWidget>(UIM->PushWidgetToLayer(GYUILayerTags::UI_Layer_Modal, PopupClass));
				}
			}
		}
	}

	// 2) 메뉴 / 레이아웃 없음 / 트래블 중 → 뷰포트 최상단(ZOrder 10000)에 직접
	if (!Popup)
	{
		Popup = PC ? CreateWidget<UGYMessagePopupWidget>(PC, PopupClass)
				   : CreateWidget<UGYMessagePopupWidget>(World, PopupClass);
		if (Popup) Popup->AddToViewport(10000);
	}
	// 메뉴 / 레이아웃 없음 / 트래블 중 뷰포트 최상단에 직접
	if (!Popup)
	{
		Popup = PC
		? CreateWidget<UGYMessagePopupWidget>(PC, PopupClass)
		: CreateWidget<UGYMessagePopupWidget>(World, PopupClass);
		if (Popup) Popup->AddToViewport(10000);
	}
	if (Popup)
	{
		if (bConfirm) Popup->SetupConfirm(Title, Message);
		else          Popup->SetupNotice(Title, Message);
	}
	return Popup;
}

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

static FAutoConsoleCommandWithWorld GYCmd_TestNotice(
	TEXT("gy.UI.TestNotice"),
	TEXT("안내(닫기 전용) 팝업 표시 테스트"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UGYMessagePopupWidget::ShowNotice(World,
			NSLOCTEXT("GYUI", "Test_Notice_Title", "안내 테스트"),
			NSLOCTEXT("GYUI", "Test_Notice_Msg", "닫기 전용 팝업입니다."));
	}));

static FAutoConsoleCommandWithWorld GYCmd_TestConfirm(
	TEXT("gy.UI.TestConfirm"),
	TEXT("확인(네/아니오) 팝업 표시 테스트"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UGYMessagePopupWidget::ShowConfirm(World,
			NSLOCTEXT("GYUI", "Test_Confirm_Title", "확인 테스트"),
			NSLOCTEXT("GYUI", "Test_Confirm_Msg", "네/아니오 팝업입니다."));
	}));
#endif
