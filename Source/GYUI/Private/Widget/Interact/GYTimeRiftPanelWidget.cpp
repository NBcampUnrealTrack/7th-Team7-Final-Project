#include "Widget/Interact/GYTimeRiftPanelWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Player/GYPlayerState.h"

UGYTimeRiftPanelWidget::UGYTimeRiftPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// ESC(뒤로가기) 액션을 이 위젯이 처리하도록
	bIsBackHandler = true;
}

void UGYTimeRiftPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// ESC 키 입력을 받기 위해 포커스 가능하도록
	SetIsFocusable(true);
}

FReply UGYTimeRiftPanelWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RequestExit();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UGYTimeRiftPanelWidget::NativeOnHandleBackAction()
{
	RequestExit();
	return true;
}

UWidget* UGYTimeRiftPanelWidget::NativeGetDesiredFocusTarget() const
{
	return const_cast<UGYTimeRiftPanelWidget*>(this);
}

void UGYTimeRiftPanelWidget::RequestExit()
{
	const FGameplayTag ExitEvent = GetExitEventTag();
	if (!ExitEvent.IsValid()) return;

	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;

	ASC->Server_SendGameplayEvent(ExitEvent, FGameplayEventData());
}
