#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;
class SEditableTextBox;

struct FForceUnlockDialogResult
{
	bool bConfirmed = false;
	FString Reason;
};

class SForceUnlockDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SForceUnlockDialog) {}
		SLATE_ARGUMENT(FString, RelativePath)
		SLATE_ARGUMENT(FString, OwnerId)
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(FForceUnlockDialogResult*, OutResult)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TWeakPtr<SWindow> ParentWindow;
	FForceUnlockDialogResult* OutResult = nullptr;
	TSharedPtr<SEditableTextBox> ReasonBox;
	bool bSecondConfirmShown = false;

	FReply HandlePrimary();
	FReply HandleCancel();
	void CloseParent();
};
