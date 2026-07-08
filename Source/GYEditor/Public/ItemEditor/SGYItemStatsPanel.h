#pragma once

#include "CoreMinimal.h"
#include "DataTableEditorUtils.h"
#include "Misc/NotifyHook.h"
#include "ScopedTransaction.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class FGYItemEditorController;
class IStructureDetailsView;
class SVerticalBox;
class UDataTable;
class UItemDefinition;

// 베이스 스탯 DT 행을 아이템 상세 화면에서 직접 편집하는 패널.
// DT 에디터와 동시에 열려 있어도 동기화되도록 FDataTableEditorUtils 브로드캐스트를 주고받는다.
class SGYItemStatsPanel
	: public SCompoundWidget
	, public FNotifyHook
	, public FDataTableEditorUtils::INotifyOnDataTableChanged
{
public:
	SLATE_BEGIN_ARGS(SGYItemStatsPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController);
	virtual ~SGYItemStatsPanel() override;

	void SetItem(UItemDefinition* Item);
	void Refresh();

	// FNotifyHook — 구조체 뷰 편집을 트랜잭션으로 감싼다
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

	// INotifyOnDataTableChanged — 외부(DT 에디터)에서 행이 추가/삭제되면 행 포인터를 다시 잡는다
	virtual void PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override;
	virtual void PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override;

private:
	TSharedPtr<FGYItemEditorController> Controller;
	TWeakObjectPtr<UItemDefinition> CurrentItem;
	TWeakObjectPtr<UDataTable> CurrentTable;

	TSharedPtr<IStructureDetailsView> StructureView;
	TSharedPtr<SVerticalBox> ContentBox;
	TUniquePtr<FScopedTransaction> ActiveTransaction;
	bool bBroadcastingChange = false;
};
