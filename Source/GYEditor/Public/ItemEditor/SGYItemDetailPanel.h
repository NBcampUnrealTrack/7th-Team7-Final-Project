#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class FGYItemEditorController;
class IDetailsView;
class SGYItemPoolPanel;
class SGYItemStatsPanel;
class SGYItemValidationPanel;
class UItemDefinition;

class SGYItemDetailPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGYItemDetailPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController);

	void SetItem(UItemDefinition* Item);
	void RefreshAll();
	void RefreshValidation();

private:
	TSharedPtr<FGYItemEditorController> Controller;
	TWeakObjectPtr<UItemDefinition> CurrentItem;

	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SGYItemValidationPanel> ValidationPanel;
	TSharedPtr<SGYItemStatsPanel> StatsPanel;
	TSharedPtr<SGYItemPoolPanel> PoolPanel;
};
