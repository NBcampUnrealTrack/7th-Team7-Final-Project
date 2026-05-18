#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;

enum class EBlockedDialogResult : uint8
{
	Confirmed,
	ForceUnlockRequested,
};

class SBlockedDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBlockedDialog) {}
		SLATE_ARGUMENT(FString, RelativePath)
		SLATE_ARGUMENT(FString, OwnerId)
		SLATE_ARGUMENT(FString, ElapsedText)
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(EBlockedDialogResult*, OutResult)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TWeakPtr<SWindow> ParentWindow;
	EBlockedDialogResult* OutResult = nullptr;

	FReply HandleConfirm();
	FReply HandleForceUnlock();
	void CloseParent();
};
