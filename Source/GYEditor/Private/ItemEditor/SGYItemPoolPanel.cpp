#include "ItemEditor/SGYItemPoolPanel.h"

#include "ItemEditor/GYItemEditorController.h"

#include "Items/ItemDefinition.h"

#include "Engine/DataTable.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

void SGYItemPoolPanel::Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController)
{
	Controller = InController;

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
				.Text(LOCTEXT("PoolHeader", "드랍 설정"))
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

void SGYItemPoolPanel::SetItem(UItemDefinition* Item)
{
	CurrentItem = Item;
	Refresh();
}

void SGYItemPoolPanel::Refresh()
{
	ContentBox->ClearChildren();

	UItemDefinition* Item = CurrentItem.Get();
	if (Item == nullptr)
	{
		ContentBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PoolNoSelection", "아이템을 선택하세요."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		return;
	}

	const TArray<FGYPoolMembership> Memberships = Controller->QueryPoolMembership(Item);
	if (Memberships.Num() == 0)
	{
		ContentBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoPools", "프로젝트에 아이템 풀 DT가 없습니다."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		return;
	}

	for (const FGYPoolMembership& Membership : Memberships)
	{
		UDataTable* Pool = Membership.Pool.Get();
		if (!IsValid(Pool)) continue;

		TWeakObjectPtr<UDataTable> PoolWeak = Pool;

		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(Membership.bMember ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, PoolWeak](ECheckBoxState State)
			{
				UItemDefinition* PinnedItem = CurrentItem.Get();
				UDataTable* PinnedPool = PoolWeak.Get();
				if (PinnedItem != nullptr && IsValid(PinnedPool))
				{
					Controller->SetPoolMembership(PinnedItem, PinnedPool, State == ECheckBoxState::Checked);
				}
			})
			[
				SNew(STextBlock).Text(FText::FromString(Pool->GetName()))
			]
		];

		if (Membership.bMember)
		{
			Row->AddSlot()
			.FillWidth(1.f)
			[
				SNew(SSpacer)
			];
			Row->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.f, 0.f, 0.f, 0.f)
			[
				MakeCountBox(LOCTEXT("Weight", "가중치"), PoolWeak, &FGYPoolRowValues::Weight)
			];
			Row->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.f, 0.f, 0.f, 0.f)
			[
				MakeCountBox(LOCTEXT("MinCount", "최소"), PoolWeak, &FGYPoolRowValues::MinCount)
			];
			Row->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.f, 0.f, 0.f, 0.f)
			[
				MakeCountBox(LOCTEXT("MaxCount", "최대"), PoolWeak, &FGYPoolRowValues::MaxCount)
			];
		}

		ContentBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 2.f)
		[
			Row
		];
	}

	const TArray<FText> RegionNames = Controller->QueryRegionExposure(Item);

	FText RegionText;
	FSlateColor RegionColor = FSlateColor::UseSubduedForeground();
	if (RegionNames.Num() > 0)
	{
		RegionText = FText::Format(LOCTEXT("ExposedRegions", "노출 Region: {0}"),
			FText::Join(FText::FromString(TEXT(", ")), RegionNames));
	}
	else
	{
		RegionText = LOCTEXT("NoExposedRegions", "노출 Region 없음 — 이 아이템은 드랍되지 않습니다.");
		RegionColor = FSlateColor(FLinearColor(1.f, 0.75f, 0.25f));
	}

	ContentBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Text(RegionText)
		.ColorAndOpacity(RegionColor)
		.AutoWrapText(true)
	];
}

TSharedRef<SWidget> SGYItemPoolPanel::MakeCountBox(const FText& Label, TWeakObjectPtr<UDataTable> PoolWeak, int32 FGYPoolRowValues::* Field)
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.Padding(0.f, 0.f, 4.f, 0.f)
	[
		SNew(STextBlock)
		.Text(Label)
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(56.f)
		[
			SNew(SNumericEntryBox<int32>)
			.MinValue(1)
			.Value_Lambda([this, PoolWeak, Field]() -> TOptional<int32>
			{
				const UItemDefinition* PinnedItem = CurrentItem.Get();
				UDataTable* PinnedPool = PoolWeak.Get();
				if (PinnedItem == nullptr || !IsValid(PinnedPool)) return TOptional<int32>();

				const TOptional<FGYPoolRowValues> Values = Controller->GetPoolRowValues(PinnedItem, PinnedPool);
				if (!Values.IsSet()) return TOptional<int32>();

				return Values.GetValue().*Field;
			})
			.OnValueCommitted_Lambda([this, PoolWeak, Field](int32 NewValue, ETextCommit::Type)
			{
				UItemDefinition* PinnedItem = CurrentItem.Get();
				UDataTable* PinnedPool = PoolWeak.Get();
				if (PinnedItem == nullptr || !IsValid(PinnedPool)) return;

				TOptional<FGYPoolRowValues> Values = Controller->GetPoolRowValues(PinnedItem, PinnedPool);
				if (!Values.IsSet()) return;

				FGYPoolRowValues NewValues = Values.GetValue();
				NewValues.*Field = NewValue;
				Controller->SetPoolRowValues(PinnedItem, PinnedPool, NewValues);
			})
		]
	];
}

#undef LOCTEXT_NAMESPACE
