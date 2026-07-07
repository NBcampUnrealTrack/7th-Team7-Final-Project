#include "ItemEditor/SGYItemStatsPanel.h"

#include "ItemEditor/GYItemEditorController.h"

#include "Items/ItemDefinition.h"

#include "Engine/DataTable.h"
#include "IStructureDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "UObject/StructOnScope.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

void SGYItemStatsPanel::Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController)
{
	Controller = InController;

	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = false;
	DetailsArgs.NotifyHook = this;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FStructureDetailsViewArgs StructureArgs;

	StructureView = PropertyModule.CreateStructureDetailView(DetailsArgs, StructureArgs, nullptr);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StatsHeader", "베이스 스탯"))
				.Font(FAppStyle::GetFontStyle("BoldFont"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(ContentBox, SVerticalBox)
			]
		]
	];

	Refresh();
}

SGYItemStatsPanel::~SGYItemStatsPanel()
{
	ActiveTransaction.Reset();
}

void SGYItemStatsPanel::SetItem(UItemDefinition* Item)
{
	CurrentItem = Item;
	Refresh();
}

void SGYItemStatsPanel::Refresh()
{
	ContentBox->ClearChildren();
	StructureView->SetStructureData(nullptr);
	CurrentTable = nullptr;

	UItemDefinition* Item = CurrentItem.Get();
	if (Item == nullptr)
	{
		ContentBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StatsNoSelection", "아이템을 선택하세요."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		return;
	}

	UDataTable* Table = Controller->ResolveBaseStatsTable(Item);
	if (!IsValid(Table))
	{
		ContentBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StatsNotApplicable", "무기/방어구 fragment가 없어 베이스 스탯 대상이 아닙니다."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.AutoWrapText(true)
		];
		return;
	}

	CurrentTable = Table;

	uint8* Row = Table->GetRowMap().FindRef(Item->ItemId);
	if (Row == nullptr)
	{
		ContentBox->AddSlot()
		.AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("StatsRowMissing", "{0}에 ItemId '{1}' 행이 없습니다."),
					FText::FromString(Table->GetName()), FText::FromName(Item->ItemId)))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateStatsRow", "행 생성"))
				.IsEnabled(!Item->ItemId.IsNone())
				.OnClicked_Lambda([this]()
				{
					if (UItemDefinition* PinnedItem = CurrentItem.Get())
					{
						Controller->CreateBaseStatsRow(PinnedItem);
					}
					return FReply::Handled();
				})
			]
		];
		return;
	}

	StructureView->SetStructureData(MakeShared<FStructOnScope>(Table->GetRowStruct(), Row));

	ContentBox->AddSlot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("%s  ·  행: %s"), *Table->GetName(), *Item->ItemId.ToString())))
		.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		.Font(FAppStyle::GetFontStyle("SmallFont"))
	];

	ContentBox->AddSlot()
	.AutoHeight()
	[
		StructureView->GetWidget().ToSharedRef()
	];

	ContentBox->AddSlot()
	.AutoHeight()
	.HAlign(HAlign_Right)
	[
		SNew(SButton)
		.Text(LOCTEXT("DeleteStatsRow", "행 삭제"))
		.OnClicked_Lambda([this]()
		{
			if (UItemDefinition* PinnedItem = CurrentItem.Get())
			{
				Controller->DeleteBaseStatsRow(PinnedItem);
			}
			return FReply::Handled();
		})
	];
}

void SGYItemStatsPanel::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	UDataTable* Table = CurrentTable.Get();
	if (!IsValid(Table)) return;

	ActiveTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("EditBaseStats", "베이스 스탯 편집"));
	Table->Modify();
}

void SGYItemStatsPanel::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	UDataTable* Table = CurrentTable.Get();
	if (IsValid(Table))
	{
		Table->MarkPackageDirty();

		bBroadcastingChange = true;
		FDataTableEditorUtils::BroadcastPostChange(Table, FDataTableEditorUtils::EDataTableChangeInfo::RowData);
		bBroadcastingChange = false;
	}

	ActiveTransaction.Reset();
}

void SGYItemStatsPanel::PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	if (bBroadcastingChange) return;
	if (Changed != CurrentTable.Get()) return;
	if (Info != FDataTableEditorUtils::EDataTableChangeInfo::RowList) return;

	// 행 추가/삭제 직전 — 들고 있던 행 포인터를 놓는다
	StructureView->SetStructureData(nullptr);
}

void SGYItemStatsPanel::PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	if (bBroadcastingChange) return;
	if (Changed != CurrentTable.Get()) return;
	if (Info != FDataTableEditorUtils::EDataTableChangeInfo::RowList) return;

	Refresh();
}

#undef LOCTEXT_NAMESPACE
