#include "Debug/GYDebugMenuManager.h"
#include "Debug/GYDebugMenu.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Player/GYPlayerState.h"
#include "GameFramework/PlayerController.h"

void FGYDebugMenuManager::Initialize()
{
	PIEStartHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FGYDebugMenuManager::OnPostPIEStarted);
	PIEEndHandle = FEditorDelegates::PrePIEEnded.AddRaw(this, &FGYDebugMenuManager::OnPrePIEEnded);
}

void FGYDebugMenuManager::Shutdown()
{
	FEditorDelegates::PostPIEStarted.Remove(PIEStartHandle);
	FEditorDelegates::PrePIEEnded.Remove(PIEEndHandle);

	if (FSlateApplication::IsInitialized())
		FSlateApplication::Get().UnregisterInputPreProcessor(AsShared());
}

void FGYDebugMenuManager::OnPostPIEStarted(bool bIsSimulating)
{
	bPIEActive = true;
	FSlateApplication::Get().RegisterInputPreProcessor(AsShared());
}

void FGYDebugMenuManager::OnPrePIEEnded(bool bIsSimulating)
{
	bPIEActive = false;

	if (bVisible && DebugMenuWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(DebugMenuWidget.ToSharedRef());
		bVisible = false;
	}
	DebugMenuWidget.Reset();

	if (FSlateApplication::IsInitialized())
		FSlateApplication::Get().UnregisterInputPreProcessor(AsShared());
}

bool FGYDebugMenuManager::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (!bPIEActive || InKeyEvent.GetKey() != EKeys::Backslash)
		return false;

	ToggleMenu();
	return true;
}

void FGYDebugMenuManager::ToggleMenu()
{
	if (!GEngine || !GEngine->GameViewport) return;

	auto GetPC = []() -> APlayerController*
	{
		const FWorldContext* Ctx = GEditor ? GEditor->GetPIEWorldContext() : nullptr;
		UWorld* World = Ctx ? Ctx->World() : nullptr;
		return World ? World->GetFirstPlayerController() : nullptr;
	};

	if (bVisible)
	{
		if (DebugMenuWidget.IsValid())
			GEngine->GameViewport->RemoveViewportWidgetContent(DebugMenuWidget.ToSharedRef());
		bVisible = false;

		if (APlayerController* PC = GetPC())
			PC->bShowMouseCursor = false;
	}
	else
	{
		AGYPlayerState* PS = nullptr;
		if (APlayerController* PC = GetPC())
		{
			PS = PC->GetPlayerState<AGYPlayerState>();
			PC->bShowMouseCursor = true;
		}

		DebugMenuWidget = SNew(SGYDebugMenu).PlayerState(PS);
		GEngine->GameViewport->AddViewportWidgetContent(DebugMenuWidget.ToSharedRef(), 100);
		bVisible = true;
	}
}
