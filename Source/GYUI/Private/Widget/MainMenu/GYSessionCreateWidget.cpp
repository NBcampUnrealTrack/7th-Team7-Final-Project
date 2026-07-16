#include "Widget/MainMenu/GYSessionCreateWidget.h"
#include "Components/Button.h"

void UGYSessionCreateWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Cancel)
		Button_Cancel->OnClicked.AddDynamic(this, &UGYSessionCreateWidget::HandleCancelClicked);
}

void UGYSessionCreateWidget::HandleCancelClicked()
{
	ClosePanel();
}
