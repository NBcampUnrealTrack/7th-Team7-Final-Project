#include "UI/SBlockedDialog.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "Poorforce"

void SBlockedDialog::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	OutResult = InArgs._OutResult;

	const FString HeaderText  = TEXT("⚠ 이 에셋은 다른 사람이 작업 중입니다");
	const FString PathText    = FString::Printf(TEXT("에셋: %s"), *InArgs._RelativePath);
	const FString OwnerText   = FString::Printf(TEXT("작업자: %s"), *InArgs._OwnerId);
	const FString ElapsedText = FString::Printf(TEXT("시작: %s"), *InArgs._ElapsedText);

	TSharedRef<SUniformGridPanel> ButtonGrid = SNew(SUniformGridPanel)
		.SlotPadding(FMargin(6.f, 0.f, 0.f, 0.f));

	ButtonGrid->AddSlot(0, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("Confirm", "확인"))
		.HAlign(HAlign_Center)
		.OnClicked(this, &SBlockedDialog::HandleConfirm)
	];

	int32 NextColumn = 1;

	if (InArgs._bShowOpenAnyway)
	{
		ButtonGrid->AddSlot(NextColumn++, 0)
		[
			SNew(SButton)
			.Text(LOCTEXT("OpenAnyway", "그래도 열기"))
			.ToolTipText(LOCTEXT("OpenAnywayTooltip",
				"락 없이 엽니다. 저장은 가능하나 push 시 LFS가 거부할 수 있습니다."))
			.HAlign(HAlign_Center)
			.OnClicked(this, &SBlockedDialog::HandleOpenAnyway)
		];
	}

	if (InArgs._bShowForceUnlock)
	{
		ButtonGrid->AddSlot(NextColumn++, 0)
		[
			SNew(SButton)
			.Text(LOCTEXT("ForceUnlock", "강제 해제..."))
			.HAlign(HAlign_Center)
			.OnClicked(this, &SBlockedDialog::HandleForceUnlock)
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(20.f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(HeaderText))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 4.f))
			[
				SNew(STextBlock).Text(FText::FromString(PathText))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 4.f))
			[
				SNew(STextBlock).Text(FText::FromString(OwnerText))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 4.f, 0.f, 16.f))
			[
				SNew(STextBlock).Text(FText::FromString(ElapsedText))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				ButtonGrid
			]
		]
	];
}

FReply SBlockedDialog::HandleConfirm()
{
	if (OutResult != nullptr) *OutResult = EBlockedDialogResult::Confirmed;
	CloseParent();
	return FReply::Handled();
}

FReply SBlockedDialog::HandleForceUnlock()
{
	if (OutResult != nullptr) *OutResult = EBlockedDialogResult::ForceUnlockRequested;
	CloseParent();
	return FReply::Handled();
}

FReply SBlockedDialog::HandleOpenAnyway()
{
	if (OutResult != nullptr) *OutResult = EBlockedDialogResult::OpenAnywayRequested;
	CloseParent();
	return FReply::Handled();
}

void SBlockedDialog::CloseParent()
{
	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
