#include "Widget/MainMenu/GYSessionContainerWidget.h"
#include "Components/Button.h"

void UGYSessionContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_CreateSession)
		Button_CreateSession->OnClicked.AddDynamic(this, &UGYSessionContainerWidget::HandleCreateSessionClicked);

	if (Button_Back)
		Button_Back->OnClicked.AddDynamic(this, &UGYSessionContainerWidget::HandleBackClicked);
}

void UGYSessionContainerWidget::HandleCreateSessionClicked()
{
	OnCreateSessionRequested.Broadcast();
}

void UGYSessionContainerWidget::HandleBackClicked()
{
	ClosePanel();
}
