#include "UI/SForceUnlockDialog.h"

#include "Misc/MessageDialog.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "Poorforce"

void SForceUnlockDialog::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	OutResult = InArgs._OutResult;

	const FString HeaderText = TEXT("강제 해제");
	const FString PathText   = FString::Printf(TEXT("에셋: %s"), *InArgs._RelativePath);
	const FString OwnerText  = FString::Printf(TEXT("현재 작업자: %s"), *InArgs._OwnerId);
	const FString WarnText   = TEXT("⚠ 작업자의 진행 중인 변경사항이 사라질 수 있습니다.");
	const FString ReasonLabel = TEXT("사유 (선택):");

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
			.Padding(FMargin(0.f, 8.f))
			[
				SNew(STextBlock).Text(FText::FromString(WarnText))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 12.f, 0.f, 4.f))
			[
				SNew(STextBlock).Text(FText::FromString(ReasonLabel))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 16.f))
			[
				SAssignNew(ReasonBox, SEditableTextBox)
				.HintText(LOCTEXT("ReasonHint", "예) 작업자 부재로 인한 회수"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.f, 0.f, 0.f, 0.f))

				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Cancel", "취소"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SForceUnlockDialog::HandleCancel)
				]

				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ForceUnlockGo", "강제 해제"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SForceUnlockDialog::HandlePrimary)
				]
			]
		]
	];
}

FReply SForceUnlockDialog::HandlePrimary()
{
	const EAppReturnType::Type Choice = FMessageDialog::Open(
		EAppMsgType::YesNo,
		LOCTEXT("FinalConfirm", "정말로 강제 해제하시겠습니까?\n현재 작업자의 변경사항이 손실될 수 있습니다."));

	if (Choice != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	if (OutResult != nullptr)
	{
		OutResult->bConfirmed = true;
		OutResult->Reason = ReasonBox.IsValid() ? ReasonBox->GetText().ToString() : FString{};
	}

	CloseParent();
	return FReply::Handled();
}

FReply SForceUnlockDialog::HandleCancel()
{
	if (OutResult != nullptr)
	{
		OutResult->bConfirmed = false;
	}
	CloseParent();
	return FReply::Handled();
}

void SForceUnlockDialog::CloseParent()
{
	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
