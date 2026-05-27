#include "Menu/GYConnectMenuWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Player/GYPlayerController.h"

UGYConnectMenuWidget::UGYConnectMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EGYWidgetInputMode::Menu;
}

void UGYConnectMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IpTextBox != nullptr && IpTextBox->GetText().IsEmpty())
	{
		IpTextBox->SetText(FText::FromString(DefaultAddress));
	}

	if (ConnectButton != nullptr)
	{
		ConnectButton->OnClicked.AddDynamic(this, &UGYConnectMenuWidget::HandleConnectClicked);
	}
}

void UGYConnectMenuWidget::HandleConnectClicked()
{
	if (IpTextBox == nullptr) return;

	const FString Address = IpTextBox->GetText().ToString();

	AGYPlayerController* PC = GetOwningPlayer<AGYPlayerController>();
	if (PC == nullptr)
	{
		PC = Cast<AGYPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	}
	if (PC == nullptr) return;

	PC->ConnectToServer(Address);
}
