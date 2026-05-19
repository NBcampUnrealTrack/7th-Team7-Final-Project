#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;
class STextBlock;
class SVerticalBox;
class FRcloneProcessManager;

class SUploadProgressWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUploadProgressWidget) {}
		SLATE_ARGUMENT(TSharedPtr<FRcloneProcessManager>, Rclone)
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(bool*, OutForceCancelled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedPtr<FRcloneProcessManager> Rclone;
	TWeakPtr<SWindow> ParentWindow;
	bool* OutForceCancelled = nullptr;

	TSharedPtr<STextBlock> HeaderText;
	TSharedPtr<SVerticalBox> ItemList;

	int32 LastShownCount = -1;

	FReply HandleForceQuit();
	void CloseParent();
};
