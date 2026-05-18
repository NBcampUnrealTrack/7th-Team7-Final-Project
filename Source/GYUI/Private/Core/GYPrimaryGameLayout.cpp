#include "Core/GYPrimaryGameLayout.h"
#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UGYPrimaryGameLayout::UGYPrimaryGameLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGYPrimaryGameLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Layers.Reset();

	// BP에서 짝지어둔 태그-이름 하나씩 체크
	for (const TPair<FGameplayTag, FName>& Pair : LayerStackBindings)
	{
		UWidget* Found = GetWidgetFromName(Pair.Value);
		UCommonActivatableWidgetContainerBase* Stack = Cast<UCommonActivatableWidgetContainerBase>(Found);

		if (Stack)
		{
			Layers.Add(Pair.Key, Stack); // 찾으면 Layer 맵에 태그-층 형태로 저장
		}
	}
}

UCommonActivatableWidgetContainerBase* UGYPrimaryGameLayout::GetLayerWidget(FGameplayTag LayerTag) const
{
	// Layer 맵에서 태그로 해당 층 찾아서 돌려줌
	if (const TObjectPtr<UCommonActivatableWidgetContainerBase>* Found = Layers.Find(LayerTag))
	{
		return *Found;
	}
	return nullptr;
}

UCommonActivatableWidget* UGYPrimaryGameLayout::PushWidgetToLayer(FGameplayTag LayerTag,
                                                                  TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!WidgetClass) return nullptr;

	// 위젯을 띄울 층 찾고 그 층에 위젯 추가 - Common UI가 알아서 화면에 띄우고 Activate 시켜줌
	if (UCommonActivatableWidgetContainerBase* Stack = GetLayerWidget(LayerTag))
	{
		return Stack->AddWidget(WidgetClass);
	}
	return nullptr;
}

void UGYPrimaryGameLayout::RemoveWidgetFromLayer(UCommonActivatableWidget* Widget)
{
	if (!Widget) return;

	for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>>& Pair : Layers)
	{
		if (Pair.Value)
		{
			Pair.Value->RemoveWidget(*Widget);
		}
	}
}

void UGYPrimaryGameLayout::ClearAllLayers()
{
	for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>>& Pair : Layers)
	{
		if (Pair.Value) Pair.Value->ClearWidgets();
	}
}
