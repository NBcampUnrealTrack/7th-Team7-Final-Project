#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class ITableRow;
class STableViewBase;
class STextBlock;

struct FPoorforceLockRow
{
	FString Key;            // 전체 Redis 락 키
	FString RelativePath;   // 표시용 (키에서 네임스페이스 제거)
	bool bChecked = false;
};

class SPoorforceLockWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPoorforceLockWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<ITableRow> OnGenerateRow(
		TSharedPtr<FPoorforceLockRow> Item,
		const TSharedRef<STableViewBase>& OwnerTable);

	void RefreshFromServer();
	void ApplyFilter();
	void OnSearchTextChanged(const FText& NewText);
	void SetStatus(const FString& Text);

	FReply OnRefreshClicked();
	FReply OnSelectAllClicked();
	FReply OnClearSelectionClicked();
	FReply OnReleaseSelectedClicked();

	TArray<TSharedPtr<FPoorforceLockRow>> AllRows;
	TArray<TSharedPtr<FPoorforceLockRow>> FilteredRows;
	FString FilterText;

	TSharedPtr<SListView<TSharedPtr<FPoorforceLockRow>>> ListView;
	TSharedPtr<STextBlock> StatusText;

	bool bBusy = false;
};
