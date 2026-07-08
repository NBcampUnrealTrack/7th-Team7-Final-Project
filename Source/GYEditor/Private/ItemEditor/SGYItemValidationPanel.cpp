#include "ItemEditor/SGYItemValidationPanel.h"

#include "ItemEditor/GYItemEditorShared.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

void SGYItemValidationPanel::Construct(const FArguments& InArgs)
{
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
				.Text(LOCTEXT("ValidationHeader", "검증 결과"))
				.Font(FAppStyle::GetFontStyle("BoldFont"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(MessageBox, SVerticalBox)
			]
		]
	];

	SetReport(nullptr);
}

void SGYItemValidationPanel::SetReport(const FGYItemValidationReport* Report)
{
	MessageBox->ClearChildren();

	if (Report == nullptr)
	{
		MessageBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoSelection", "아이템을 선택하세요."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		return;
	}

	if (!Report->HasAny())
	{
		MessageBox->AddSlot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SImage).Image(GYItemEditorUI::GetPassIcon())
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("NoIssues", "문제 없음"))
			]
		];
		return;
	}

	for (const FGYItemValidationMessage& Message : Report->Messages)
	{
		MessageBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Top)
			.Padding(0.f, 1.f, 4.f, 0.f)
			[
				SNew(SImage).Image(GYItemEditorUI::GetSeverityIcon(Message.Severity))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Message.Message)
				.AutoWrapText(true)
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
