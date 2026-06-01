#include "Core/GYOwnerAwareWidgetComponent.h"
#include "Core/GYUserWidget.h"

void UGYOwnerAwareWidgetComponent::InitWidget()
{
	Super::InitWidget();

	if (UGYUserWidget* W = Cast<UGYUserWidget>(GetUserWidgetObject()))
	{
		W->SetWidgetOwnerActor(GetOwner());
	}
}
