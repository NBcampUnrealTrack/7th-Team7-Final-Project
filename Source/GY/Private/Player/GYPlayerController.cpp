#include "Player/GYPlayerController.h"
#include "Cheats/GYCheatManager.h"

#if !UE_BUILD_SHIPPING
#include "Debug/GYDebugMenu.h"
#include "Player/GYPlayerState.h"
#include "Engine/GameViewportClient.h"
#endif

AGYPlayerController::AGYPlayerController()
{
	CheatClass = UGYCheatManager::StaticClass();
}

void AGYPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

#if !UE_BUILD_SHIPPING
	InputComponent->BindKey(EKeys::Tilde, IE_Pressed, this, &AGYPlayerController::ToggleDebugMenu);
#endif
}

#if !UE_BUILD_SHIPPING
void AGYPlayerController::ToggleDebugMenu()
{
	if (!GEngine || !GEngine->GameViewport) return;

	if (bDebugMenuVisible)
	{
		if (DebugMenuWidget.IsValid())
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(DebugMenuWidget.ToSharedRef());
		}
		bDebugMenuVisible = false;
		bShowMouseCursor = false;
	}
	else
	{
		AGYPlayerState* PS = GetPlayerState<AGYPlayerState>();
		DebugMenuWidget = SNew(SGYDebugMenu).PlayerState(PS);
		GEngine->GameViewport->AddViewportWidgetContent(DebugMenuWidget.ToSharedRef(), 100);
		bDebugMenuVisible = true;
		bShowMouseCursor = true;
	}
}
#endif
