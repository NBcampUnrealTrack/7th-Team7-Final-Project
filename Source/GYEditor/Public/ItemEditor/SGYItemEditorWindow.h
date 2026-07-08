#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FGYItemEditorController;
class SGYItemBrowserPanel;
class SGYItemDetailPanel;
class UItemDefinition;

class SGYItemEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGYItemEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SGYItemEditorWindow() override;

private:
	void HandleDataChanged();
	void HandleItemSelected(UItemDefinition* Item);

	TSharedPtr<FGYItemEditorController> Controller;
	TSharedPtr<SGYItemBrowserPanel> BrowserPanel;
	TSharedPtr<SGYItemDetailPanel> DetailPanel;
};
