#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

class SGYDebugMenu;

class FGYDebugMenuManager : public IInputProcessor, public TSharedFromThis<FGYDebugMenuManager>
{
public:
	void Initialize();
	void Shutdown();

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

private:
	void OnPostPIEStarted(bool bIsSimulating);
	void OnPrePIEEnded(bool bIsSimulating);
	void ToggleMenu();

	TSharedPtr<SGYDebugMenu> DebugMenuWidget;
	bool bVisible = false;
	bool bPIEActive = false;
	FDelegateHandle PIEStartHandle;
	FDelegateHandle PIEEndHandle;
};
