#include "Menu/GYConnectMenuWidget.h"

#include "Logging/GYLogManager.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/GYPlayerController.h"

namespace
{
	// dev: IP 직접 입력 접속 — 메인메뉴에서 빠진 뒤로는 이 콘솔로만 띄운다 (재실행 = 토글)
	void ConnectMenuCmd(const TArray<FString>& Args, UWorld* World)
	{
		if (!IsValid(World)) return;
		APlayerController* PC = World->GetFirstPlayerController();
		if (!IsValid(PC)) return;

		TArray<UUserWidget*> Existing;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Existing, UGYConnectMenuWidget::StaticClass(), false);
		if (Existing.Num() > 0)
		{
			Existing[0]->RemoveFromParent();
			return;
		}

		UClass* MenuClass = LoadClass<UGYConnectMenuWidget>(nullptr,
			TEXT("/Game/GY/UI/MainMenu/WBP_ConnectMenu.WBP_ConnectMenu_C"));
		if (MenuClass == nullptr)
		{
			GY_WARN(Network, KDY, "gy.UI.ConnectMenu: WBP_ConnectMenu not found");
			return;
		}

		UGYConnectMenuWidget* Menu = CreateWidget<UGYConnectMenuWidget>(PC, MenuClass);
		if (Menu != nullptr)
		{
			Menu->AddToViewport(10);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GYConnectMenuCommand(
		TEXT("gy.UI.ConnectMenu"),
		TEXT("Toggle the direct IP connect menu (dev)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ConnectMenuCmd));
}

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
