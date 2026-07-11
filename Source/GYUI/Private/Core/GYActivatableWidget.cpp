#include "Core/GYActivatableWidget.h"
#include "CommonInputTypeEnum.h"
#include "Core/GYUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Player/GYPlayerState.h"

UGYActivatableWidget::UGYActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TOptional<FUIInputConfig> UGYActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputMode)
	{
	case EGYWidgetInputMode::Game: // 게임 모드 - 조작o
		// 캡처(클릭/차지) 중에도 커서 유지
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode, false);
	case EGYWidgetInputMode::Menu: // 메뉴 모드 - 조작x, UI만 조작
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
	case EGYWidgetInputMode::Default:
	default:
		return TOptional<FUIInputConfig>();
	}
}

void UGYActivatableWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TagDrivenWidgets.Num() == 0) return;

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!LocalPlayer) return;

	UGYUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UGYUIManagerSubsystem>();
	if (!UIManager) return;

	for (const FGYTagDrivenWidgetEntry& Entry : TagDrivenWidgets)
	{
		if (Entry.StateTag.IsValid() && Entry.WidgetClass)
		{
			UIManager->RegisterTagDrivenWidget(Entry.StateTag, Entry.LayerTag, Entry.WidgetClass, Entry.bBlocksOtherWidgets);
		}
	}
}

FReply UGYActivatableWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape) { HandleBackAction(); return FReply::Handled(); }
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UGYActivatableWidget::NativeOnHandleBackAction()
{
	HandleBackAction();
	return true;
}

void UGYActivatableWidget::HandleBackAction()
{
	if (BackActionEventTag.IsValid())
	{
		SendServerGameplayEvent(BackActionEventTag);
		return;
	}
	RequestUIBack();
}

void UGYActivatableWidget::RequestUIBack()
{
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
		if (UGYUIManagerSubsystem* UI = LP->GetSubsystem<UGYUIManagerSubsystem>())
			UI->HandleUIBack();
}

void UGYActivatableWidget::SendServerGameplayEvent(FGameplayTag EventTag)
{
	if (!EventTag.IsValid()) return;
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;
	ASC->Server_SendGameplayEvent(EventTag, FGameplayEventData());
}
