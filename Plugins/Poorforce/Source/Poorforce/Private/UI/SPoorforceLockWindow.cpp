#include "UI/SPoorforceLockWindow.h"

#include "Poorforce.h"
#include "LockServerClient.h"
#include "PoorforceConfig.h"
#include "PoorforcePathResolver.h"
#include "GitLfsClient.h"
#include "UserIdProvider.h"
#include "PoorforceLog.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/STableRow.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "PoorforceLockWindow"

void SPoorforceLockWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)

		// 검색 + 새로고침
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SSearchBox)
				.HintText(LOCTEXT("SearchHint", "검색 (부분일치)..."))
				.OnTextChanged(this, &SPoorforceLockWindow::OnSearchTextChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "새로고침"))
				.OnClicked(this, &SPoorforceLockWindow::OnRefreshClicked)
			]
		]

		// 상태 텍스트
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f, 0.f, 6.f, 4.f)
		[
			SAssignNew(StatusText, STextBlock)
			.Text(LOCTEXT("Loading", "불러오는 중..."))
		]

		// 락 목록
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(6.f, 0.f, 6.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SAssignNew(ListView, SListView<TSharedPtr<FPoorforceLockRow>>)
				.ListItemsSource(&FilteredRows)
				.OnGenerateRow(this, &SPoorforceLockWindow::OnGenerateRow)
				.SelectionMode(ESelectionMode::None)
			]
		]

		// 하단 액션
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("SelectAll", "전체 선택"))
				.OnClicked(this, &SPoorforceLockWindow::OnSelectAllClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ClearSel", "선택 해제"))
				.OnClicked(this, &SPoorforceLockWindow::OnClearSelectionClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNullWidget::NullWidget
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("ReleaseSel", "락 해제"))
				.OnClicked(this, &SPoorforceLockWindow::OnReleaseSelectedClicked)
			]
		]
	];

	RefreshFromServer();
}

TSharedRef<ITableRow> SPoorforceLockWindow::OnGenerateRow(
	TSharedPtr<FPoorforceLockRow> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FPoorforceLockRow>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4.f, 2.f)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([Item]()
			{
				return Item->bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([Item](ECheckBoxState NewState)
			{
				Item->bChecked = (NewState == ECheckBoxState::Checked);
			})
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.Padding(4.f, 2.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Item->RelativePath))
		]
	];
}

void SPoorforceLockWindow::RefreshFromServer()
{
	if (bBusy) return;

	FPoorforceModule& Module = FPoorforceModule::Get();
	TSharedPtr<FLockServerClient> Client = Module.GetLockClient();
	if (!Client.IsValid())
	{
		AllRows.Reset();
		FilteredRows.Reset();
		if (ListView.IsValid()) ListView->RequestListRefresh();
		SetStatus(TEXT("Poorforce 미초기화 (PoorforceConfig.json 확인)"));
		return;
	}

	const FPoorforceConfig& Config = Module.GetConfig();
	const FString MatchPattern = Config.LockKeyNamespace.IsEmpty()
		? TEXT("asset:*")
		: FString::Printf(TEXT("%s:asset:*"), *Config.LockKeyNamespace);
	const FString Owner = PoorforceUserId::Get();

	bBusy = true;
	SetStatus(TEXT("Redis 조회 중..."));

	TWeakPtr<SPoorforceLockWindow> WeakSelf = SharedThis(this);
	Client->ListOwnedKeys(MatchPattern, Owner,
		[WeakSelf](bool bSuccess, const TArray<FString>& Keys)
		{
			TSharedPtr<SPoorforceLockWindow> Self = WeakSelf.Pin();
			if (!Self.IsValid()) return;

			Self->bBusy = false;

			if (!bSuccess)
			{
				Self->SetStatus(TEXT("조회 실패 (네트워크/서버 오류)"));
				return;
			}

			Self->AllRows.Reset();
			for (const FString& Key : Keys)
			{
				TSharedPtr<FPoorforceLockRow> Row = MakeShared<FPoorforceLockRow>();
				Row->Key = Key;
				Row->RelativePath = PoorforcePathResolver::ExtractRelativePathFromKey(Key);
				Self->AllRows.Add(Row);
			}

			Self->ApplyFilter();
			Self->SetStatus(FString::Printf(TEXT("총 %d개 락 (%s)"), Self->AllRows.Num(), *PoorforceUserId::Get()));
		});
}

void SPoorforceLockWindow::ApplyFilter()
{
	FilteredRows.Reset();

	if (FilterText.IsEmpty())
	{
		FilteredRows = AllRows;
	}
	else
	{
		const FString Needle = FilterText.ToLower();
		for (const TSharedPtr<FPoorforceLockRow>& Row : AllRows)
		{
			if (Row->RelativePath.ToLower().Contains(Needle))
			{
				FilteredRows.Add(Row);
			}
		}
	}

	if (ListView.IsValid()) ListView->RequestListRefresh();
}

void SPoorforceLockWindow::OnSearchTextChanged(const FText& NewText)
{
	FilterText = NewText.ToString();
	ApplyFilter();
}

void SPoorforceLockWindow::SetStatus(const FString& Text)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Text));
	}
}

FReply SPoorforceLockWindow::OnRefreshClicked()
{
	RefreshFromServer();
	return FReply::Handled();
}

FReply SPoorforceLockWindow::OnSelectAllClicked()
{
	for (const TSharedPtr<FPoorforceLockRow>& Row : FilteredRows)
	{
		Row->bChecked = true;
	}
	if (ListView.IsValid()) ListView->RequestListRefresh();
	return FReply::Handled();
}

FReply SPoorforceLockWindow::OnClearSelectionClicked()
{
	for (const TSharedPtr<FPoorforceLockRow>& Row : AllRows)
	{
		Row->bChecked = false;
	}
	if (ListView.IsValid()) ListView->RequestListRefresh();
	return FReply::Handled();
}

FReply SPoorforceLockWindow::OnReleaseSelectedClicked()
{
	if (bBusy) return FReply::Handled();

	FPoorforceModule& Module = FPoorforceModule::Get();
	TSharedPtr<FLockServerClient> Client = Module.GetLockClient();
	if (!Client.IsValid())
	{
		SetStatus(TEXT("Poorforce 미초기화"));
		return FReply::Handled();
	}

	TArray<TSharedPtr<FPoorforceLockRow>> Selected;
	for (const TSharedPtr<FPoorforceLockRow>& Row : AllRows)
	{
		if (Row->bChecked) Selected.Add(Row);
	}

	if (Selected.Num() == 0)
	{
		SetStatus(TEXT("선택된 락 없음"));
		return FReply::Handled();
	}

	const TArray<FPoorforceManagedPath> ManagedPaths = Module.GetConfig().ManagedPaths;

	bBusy = true;
	SetStatus(FString::Printf(TEXT("%d개 해제 중..."), Selected.Num()));

	TSharedPtr<int32> Pending = MakeShared<int32>(Selected.Num());
	TWeakPtr<SPoorforceLockWindow> WeakSelf = SharedThis(this);

	for (const TSharedPtr<FPoorforceLockRow>& Row : Selected)
	{
		const FString Key = Row->Key;
		const FString RelativePath = Row->RelativePath;

		// LFS unlock 시도 (best-effort) — 디스크에 파일 있어야 성공
		FString GitPath;
		if (PoorforcePathResolver::ReconstructLockOnlyGitPath(RelativePath, ManagedPaths, GitPath))
		{
			PoorforceGitLfs::TryUnlock(GitPath);
		}
		else
		{
			UE_LOG(LogPoorforce, Warning, TEXT("LFS unlock 스킵 — 파일 경로 역추적 실패: %s"), *RelativePath);
		}

		// Redis 해제
		Client->Release(Key,
			[WeakSelf, Pending, Key](bool bReleased)
			{
				if (!bReleased)
				{
					UE_LOG(LogPoorforce, Warning, TEXT("Redis 락 해제 실패: %s"), *Key);
				}

				if (--(*Pending) > 0) return;

				TSharedPtr<SPoorforceLockWindow> Self = WeakSelf.Pin();
				if (!Self.IsValid()) return;

				Self->bBusy = false;
				Self->RefreshFromServer();
			});
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
