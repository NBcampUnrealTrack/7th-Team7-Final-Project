#include "Widget/Interaction/GYInteractionWaitingWidget.h"
#include "Components/TextBlock.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

UGYInteractionWaitingWidget::UGYInteractionWaitingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EGYWidgetInputMode::Default;
	CountFormat = NSLOCTEXT("Ending", "WaitingFormat", "{0} / {1}");
}

void UGYInteractionWaitingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		// 대기 중 메시지 구독
		UGameplayMessageSubsystem& MSG = UGameplayMessageSubsystem::Get(World);
		ListenerHandle = MSG.RegisterListener(
			GYGameplayTags::Message_Ending_WaitingForPlayers, this, &UGYInteractionWaitingWidget::HandleWaitingMessage);
	}
}

void UGYInteractionWaitingWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem::Get(World).UnregisterListener(ListenerHandle);
	}
	Super::NativeDestruct();
}

void UGYInteractionWaitingWidget::SetCount(int32 Current, int32 Required)
{
	if (CountText)
	{
		CountText->SetText(FText::Format(CountFormat, FText::AsNumber(Current), FText::AsNumber(Required)));
	}
	OnCountUpdated(Current, Required);
}

void UGYInteractionWaitingWidget::HandleWaitingMessage(FGameplayTag, const FGYInteractionWaitingMessage& Msg)
{
	SetCount(Msg.CurrentCount, Msg.RequiredCount); // 텍스트 갱신
}
