#include "ItemEditor/SGYItemBrowserPanel.h"

#include "ItemEditor/GYItemEditorController.h"
#include "ItemEditor/GYItemEditorShared.h"

#include "Core/GameplayTags/EquipmentTags.h"
#include "Items/ItemDefinition.h"
#include "Items/Fragments/ItemFragment_Equippable.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

namespace
{
	const FString SlotFilterAll(TEXT("전체"));
	const FString SlotFilterWeapon(TEXT("무기"));
	const FString SlotFilterOutfit(TEXT("Outfit"));
	const FString SlotFilterHelmet(TEXT("Helmet"));
	const FString SlotFilterAccessory(TEXT("Accessory"));
	const FString SlotFilterNonEquipment(TEXT("비장비"));
}

void SGYItemBrowserPanel::Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController)
{
	Controller = InController;
	OnItemSelected = InArgs._OnItemSelected;

	SlotFilterOptions.Add(MakeShared<FString>(SlotFilterAll));
	SlotFilterOptions.Add(MakeShared<FString>(SlotFilterWeapon));
	SlotFilterOptions.Add(MakeShared<FString>(SlotFilterOutfit));
	SlotFilterOptions.Add(MakeShared<FString>(SlotFilterHelmet));
	SlotFilterOptions.Add(MakeShared<FString>(SlotFilterAccessory));
	SlotFilterOptions.Add(MakeShared<FString>(SlotFilterNonEquipment));
	CurrentSlotFilter = SlotFilterOptions[0];

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SNew(SSearchBox)
			.HintText(LOCTEXT("SearchHint", "이름 또는 ItemId 검색"))
			.OnTextChanged(this, &SGYItemBrowserPanel::HandleSearchChanged)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f, 0.f, 4.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&SlotFilterOptions)
				.InitiallySelectedItem(CurrentSlotFilter)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
				{
					return SNew(STextBlock).Text(FText::FromString(*Option));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Option, ESelectInfo::Type)
				{
					CurrentSlotFilter = Option;
					ApplyFilter();
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::FromString(CurrentSlotFilter.IsValid() ? *CurrentSlotFilter : SlotFilterAll);
					})
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
				{
					bProblemsOnly = (State == ECheckBoxState::Checked);
					ApplyFilter();
				})
				[
					SNew(STextBlock).Text(LOCTEXT("ProblemsOnly", "문제만 보기"))
				]
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(4.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(0.f)
			[
				SAssignNew(ListView, SListView<TSharedPtr<FGYItemBrowserEntry>>)
				.ListItemsSource(&FilteredEntries)
				.OnGenerateRow(this, &SGYItemBrowserPanel::GenerateRow)
				.OnSelectionChanged(this, &SGYItemBrowserPanel::HandleSelectionChanged)
				.SelectionMode(ESelectionMode::Single)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SNew(SExpandableArea)
			.InitiallyCollapsed(true)
			.HeaderContent()
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::Format(LOCTEXT("GlobalIssuesHeader", "전역 이슈 ({0})"), FText::AsNumber(GlobalIssueCount));
				})
				.Font(FAppStyle::GetFontStyle("BoldFont"))
			]
			.BodyContent()
			[
				SAssignNew(GlobalIssueBox, SVerticalBox)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "새로고침"))
				.OnClicked_Lambda([this]()
				{
					Controller->RefreshAll();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ValidateAll", "전체 검증"))
				.OnClicked_Lambda([this]()
				{
					Controller->ValidateAll();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("SaveAll", "모두 저장"))
				.OnClicked_Lambda([this]()
				{
					Controller->SaveAllDirty();
					return FReply::Handled();
				})
			]
		]
	];

	RefreshList();
}

void SGYItemBrowserPanel::RefreshList()
{
	RebuildGlobalIssues();
	ApplyFilter();
}

TSharedRef<ITableRow> SGYItemBrowserPanel::GenerateRow(
	TSharedPtr<FGYItemBrowserEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	UItemDefinition* Item = Entry->Item.Get();
	if (!IsValid(Item))
	{
		return SNew(STableRow<TSharedPtr<FGYItemBrowserEntry>>, OwnerTable)
			[ SNew(STextBlock).Text(LOCTEXT("InvalidItem", "(삭제된 아이템)")) ];
	}

	const FGYItemValidationReport* Report = Controller->FindReport(Item);
	const bool bHasIssues = Report != nullptr && Report->HasAny();
	const FSlateBrush* BadgeBrush = bHasIssues
		? GYItemEditorUI::GetSeverityIcon(Report->GetMaxSeverity())
		: GYItemEditorUI::GetPassIcon();

	FText Tooltip = LOCTEXT("NoIssuesTooltip", "문제 없음");
	if (bHasIssues)
	{
		TArray<FText> Lines;
		for (const FGYItemValidationMessage& Message : Report->Messages)
		{
			Lines.Add(Message.Message);
		}
		Tooltip = FText::Join(FText::FromString(TEXT("\n")), Lines);
	}

	const FText NameText = Item->DisplayName.IsEmpty()
		? FText::FromString(Item->GetName())
		: Item->DisplayName;

	FText SlotText;
	if (const UItemFragment_Equippable* Equippable = Item->FindFragment<UItemFragment_Equippable>())
	{
		if (Equippable->SlotTag.IsValid())
		{
			TArray<FString> TagParts;
			Equippable->SlotTag.GetTagName().ToString().ParseIntoArray(TagParts, TEXT("."));
			SlotText = FText::FromString(TagParts.Num() > 0 ? TagParts.Last() : FString());
		}
	}

	return SNew(STableRow<TSharedPtr<FGYItemBrowserEntry>>, OwnerTable)
	.Padding(FMargin(4.f, 3.f))
	.ToolTipText(Tooltip)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SImage).Image(BadgeBrush)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(NameText)
				.Font(FAppStyle::GetFontStyle("BoldFont"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s  ·  %s"), *Item->ItemId.ToString(), *Item->GetName())))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Font(FAppStyle::GetFontStyle("SmallFont"))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(SlotText)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
	];
}

void SGYItemBrowserPanel::HandleSelectionChanged(TSharedPtr<FGYItemBrowserEntry> Entry, ESelectInfo::Type SelectInfo)
{
	UItemDefinition* Item = Entry.IsValid() ? Entry->Item.Get() : nullptr;
	SelectedItem = Item;

	// Direct는 갱신 후 재선택 — 이미 상세 패널이 같은 아이템을 보고 있음
	if (SelectInfo != ESelectInfo::Direct)
	{
		OnItemSelected.ExecuteIfBound(Item);
	}
}

void SGYItemBrowserPanel::HandleSearchChanged(const FText& NewText)
{
	SearchString = NewText.ToString();
	ApplyFilter();
}

bool SGYItemBrowserPanel::PassesFilter(const UItemDefinition& Item) const
{
	if (bProblemsOnly)
	{
		const FGYItemValidationReport* Report = Controller->FindReport(&Item);
		const bool bHasProblem = Report != nullptr
			&& (Report->HasSeverity(EGYItemValidationSeverity::Error)
				|| Report->HasSeverity(EGYItemValidationSeverity::Warning));
		if (!bHasProblem) return false;
	}

	if (CurrentSlotFilter.IsValid() && *CurrentSlotFilter != SlotFilterAll)
	{
		const UItemFragment_Equippable* Equippable = Item.FindFragment<UItemFragment_Equippable>();
		const FGameplayTag SlotTag = Equippable != nullptr ? Equippable->SlotTag : FGameplayTag();

		bool bMatches = false;
		if (*CurrentSlotFilter == SlotFilterWeapon)
		{
			bMatches = SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Weapon);
		}
		else if (*CurrentSlotFilter == SlotFilterOutfit)
		{
			bMatches = SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Outfit);
		}
		else if (*CurrentSlotFilter == SlotFilterHelmet)
		{
			bMatches = SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Helmet);
		}
		else if (*CurrentSlotFilter == SlotFilterAccessory)
		{
			bMatches = SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Accessory1)
				|| SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Accessory2)
				|| SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Accessory3);
		}
		else if (*CurrentSlotFilter == SlotFilterNonEquipment)
		{
			bMatches = Equippable == nullptr;
		}

		if (!bMatches) return false;
	}

	if (!SearchString.IsEmpty())
	{
		const bool bMatchesSearch =
			Item.DisplayName.ToString().Contains(SearchString)
			|| Item.ItemId.ToString().Contains(SearchString)
			|| Item.GetName().Contains(SearchString);
		if (!bMatchesSearch) return false;
	}

	return true;
}

void SGYItemBrowserPanel::ApplyFilter()
{
	FilteredEntries.Reset();

	for (const TObjectPtr<UItemDefinition>& Item : Controller->GetItems())
	{
		if (!IsValid(Item)) continue;
		if (!PassesFilter(*Item)) continue;

		TSharedPtr<FGYItemBrowserEntry> Entry = MakeShared<FGYItemBrowserEntry>();
		Entry->Item = Item;
		FilteredEntries.Add(Entry);
	}

	ListView->RebuildList();

	if (SelectedItem.IsValid())
	{
		for (const TSharedPtr<FGYItemBrowserEntry>& Entry : FilteredEntries)
		{
			if (Entry->Item == SelectedItem)
			{
				ListView->SetSelection(Entry, ESelectInfo::Direct);
				break;
			}
		}
	}
}

void SGYItemBrowserPanel::RebuildGlobalIssues()
{
	const TArray<FGYItemValidationMessage>& Issues = Controller->GetGlobalIssues();
	GlobalIssueCount = Issues.Num();

	GlobalIssueBox->ClearChildren();

	if (Issues.Num() == 0)
	{
		GlobalIssueBox->AddSlot()
		.AutoHeight()
		.Padding(4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoGlobalIssues", "없음"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		return;
	}

	for (const FGYItemValidationMessage& Issue : Issues)
	{
		GlobalIssueBox->AddSlot()
		.AutoHeight()
		.Padding(4.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Top)
			.Padding(0.f, 1.f, 4.f, 0.f)
			[
				SNew(SImage).Image(GYItemEditorUI::GetSeverityIcon(Issue.Severity))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Issue.Message)
				.AutoWrapText(true)
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
