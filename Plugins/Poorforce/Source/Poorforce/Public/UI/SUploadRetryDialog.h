#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;

enum class EUploadRetryChoice : uint8
{
	Dismiss,
	Retry,
	ReleaseAnyway,
};

class SUploadRetryDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUploadRetryDialog) {}
		SLATE_ARGUMENT(FString, RelativePath)
		SLATE_ARGUMENT(int32, ExitCode)
		SLATE_ARGUMENT(FString, Output)
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(EUploadRetryChoice*, OutChoice)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TWeakPtr<SWindow> ParentWindow;
	EUploadRetryChoice* OutChoice = nullptr;

	FReply HandleRetry();
	FReply HandleReleaseAnyway();
	FReply HandleDismiss();
	void CloseParent();
};
