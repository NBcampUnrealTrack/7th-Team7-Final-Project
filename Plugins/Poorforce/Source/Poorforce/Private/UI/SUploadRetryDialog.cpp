#include "UI/SUploadRetryDialog.h"

#include "Misc/MessageDialog.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "Poorforce"

namespace
{
	constexpr int32 OutputPreviewMaxChars = 1000;

	FString TrimOutputForDisplay(const FString& In)
	{
		if (In.Len() <= OutputPreviewMaxChars) return In;
		return TEXT("...") + In.Right(OutputPreviewMaxChars);
	}
}

void SUploadRetryDialog::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	OutChoice = InArgs._OutChoice;

	const FString HeaderText  = TEXT("⚠ 업로드 실패");
	const FString PathText    = FString::Printf(TEXT("에셋: %s"), *InArgs._RelativePath);
	const FString ExitText    = FString::Printf(TEXT("종료 코드: %d"), InArgs._ExitCode);
	const FString NoticeText  = TEXT("락은 유지된 상태입니다. 어떻게 할까요?");
	const FString OutputText  = TrimOutputForDisplay(InArgs._Output);

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
				SNew(STextBlock).Text(FText::FromString(ExitText))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 8.f))
			[
				SNew(STextBlock).Text(FText::FromString(NoticeText))
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(FMargin(0.f, 12.f))
			[
				SNew(SMultiLineEditableTextBox)
				.Text(FText::FromString(OutputText))
				.IsReadOnly(true)
				.AutoWrapText(true)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(FMargin(0.f, 12.f, 0.f, 0.f))
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.f, 0.f, 0.f, 0.f))

				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Dismiss", "닫기 (락 유지)"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SUploadRetryDialog::HandleDismiss)
				]

				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ReleaseAnyway", "락 해제하고 무시"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SUploadRetryDialog::HandleReleaseAnyway)
				]

				+ SUniformGridPanel::Slot(2, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Retry", "재시도"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SUploadRetryDialog::HandleRetry)
				]
			]
		]
	];
}

FReply SUploadRetryDialog::HandleRetry()
{
	if (OutChoice != nullptr) *OutChoice = EUploadRetryChoice::Retry;
	CloseParent();
	return FReply::Handled();
}

FReply SUploadRetryDialog::HandleReleaseAnyway()
{
	const EAppReturnType::Type Choice = FMessageDialog::Open(
		EAppMsgType::YesNo,
		LOCTEXT("ReleaseConfirm", "정말로 락을 해제하시겠습니까?\n다른 사람이 구버전으로 작업을 시작할 수 있습니다."));

	if (Choice != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	if (OutChoice != nullptr) *OutChoice = EUploadRetryChoice::ReleaseAnyway;
	CloseParent();
	return FReply::Handled();
}

FReply SUploadRetryDialog::HandleDismiss()
{
	if (OutChoice != nullptr) *OutChoice = EUploadRetryChoice::Dismiss;
	CloseParent();
	return FReply::Handled();
}

void SUploadRetryDialog::CloseParent()
{
	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
