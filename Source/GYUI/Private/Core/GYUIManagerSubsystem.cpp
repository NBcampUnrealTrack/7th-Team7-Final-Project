#include "Core/GYUIManagerSubsystem.h"
#include "GYUI/Public/Core/GYPrimaryGameLayout.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

void UGYUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGYUIManagerSubsystem::Deinitialize()
{
	RemovePrimaryGameLayout();
	Super::Deinitialize();
}

bool UGYUIManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	if (IsRunningDedicatedServer()) return false;
	return true;
}

void UGYUIManagerSubsystem::CreatePrimaryGameLayout(TSubclassOf<UGYPrimaryGameLayout> LayoutClass)
{
	if (PrimaryGameLayout) return; // 중복 생성 방지
	if (!LayoutClass) return;

	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return;

	APlayerController* PC = LP->GetPlayerController(LP->GetWorld());
	if (!PC) return;

	PrimaryGameLayout = CreateWidget<UGYPrimaryGameLayout>(PC, LayoutClass);
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->AddToPlayerScreen(1000); // 항상 최상단 ZOrder
	}
}

void UGYUIManagerSubsystem::RemovePrimaryGameLayout()
{
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->RemoveFromParent(); // 뷰포트 제거
		PrimaryGameLayout = nullptr;
	}
}

UCommonActivatableWidget* UGYUIManagerSubsystem::PushWidgetToLayer(FGameplayTag LayerTag,
                                                                   TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!PrimaryGameLayout) return nullptr;

	// 레이아웃의 내부 로직으로 위젯 Push
	return PrimaryGameLayout->PushWidgetToLayer(LayerTag, WidgetClass);
}

void UGYUIManagerSubsystem::PopWidget(UCommonActivatableWidget* Widget)
{
	if (PrimaryGameLayout && Widget)
	{
		PrimaryGameLayout->RemoveWidgetFromLayer(Widget);
	}
}
