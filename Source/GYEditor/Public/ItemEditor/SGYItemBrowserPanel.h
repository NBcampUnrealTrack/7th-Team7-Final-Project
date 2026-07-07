#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FGYItemEditorController;
class SVerticalBox;
class UItemDefinition;

DECLARE_DELEGATE_OneParam(FOnGYItemSelected, UItemDefinition*);

struct FGYItemBrowserEntry
{
	TWeakObjectPtr<UItemDefinition> Item;
};

class SGYItemBrowserPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGYItemBrowserPanel) {}
		SLATE_EVENT(FOnGYItemSelected, OnItemSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController);

	void RefreshList();

private:
	TSharedRef<ITableRow> GenerateRow(TSharedPtr<FGYItemBrowserEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleSelectionChanged(TSharedPtr<FGYItemBrowserEntry> Entry, ESelectInfo::Type SelectInfo);
	void HandleSearchChanged(const FText& NewText);
	bool PassesFilter(const UItemDefinition& Item) const;
	void ApplyFilter();
	void RebuildGlobalIssues();

	TSharedPtr<FGYItemEditorController> Controller;
	FOnGYItemSelected OnItemSelected;

	TArray<TSharedPtr<FGYItemBrowserEntry>> FilteredEntries;
	TSharedPtr<SListView<TSharedPtr<FGYItemBrowserEntry>>> ListView;
	TSharedPtr<SVerticalBox> GlobalIssueBox;
	TWeakObjectPtr<UItemDefinition> SelectedItem;

	FString SearchString;
	bool bProblemsOnly = false;
	int32 GlobalIssueCount = 0;

	TArray<TSharedPtr<FString>> SlotFilterOptions;
	TSharedPtr<FString> CurrentSlotFilter;
};
