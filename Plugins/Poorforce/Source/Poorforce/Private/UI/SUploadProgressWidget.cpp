#include "UI/SUploadProgressWidget.h"

#include "RcloneProcessManager.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"

#define LOCTEXT_NAMESPACE "Poorforce"

void SUploadProgressWidget::Construct(const FArguments& InArgs)
{
	Rclone = InArgs._Rclone;
	ParentWindow = InArgs._ParentWindow;
	OutForceCancelled = InArgs._OutForceCancelled;

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
				SAssignNew(HeaderText, STextBlock)
				.Text(FText::FromString(TEXT("업로드/다운로드 진행 중...")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("완료될 때까지 잠시만 기다려주세요. 강제 종료 시 락이 남을 수 있습니다.")))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 8.f))
			[
				SNew(SProgressBar)
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(FMargin(0.f, 12.f))
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(ItemList, SVerticalBox)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(FMargin(0.f, 12.f, 0.f, 0.f))
			[
				SNew(SButton)
				.Text(LOCTEXT("ForceQuit", "강제 종료"))
				.HAlign(HAlign_Center)
				.OnClicked(this, &SUploadProgressWidget::HandleForceQuit)
			]
		]
	];
}

void SUploadProgressWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!Rclone.IsValid())
	{
		CloseParent();
		return;
	}

	const TArray<FString> Descriptions = Rclone->GetActiveDescriptions();

	if (Descriptions.Num() == 0)
	{
		CloseParent();
		return;
	}

	if (Descriptions.Num() != LastShownCount)
	{
		LastShownCount = Descriptions.Num();

		if (HeaderText.IsValid())
		{
			HeaderText->SetText(FText::FromString(
				FString::Printf(TEXT("진행 중 %d개"), Descriptions.Num())));
		}

		if (ItemList.IsValid())
		{
			ItemList->ClearChildren();
			for (const FString& Desc : Descriptions)
			{
				ItemList->AddSlot()
					.AutoHeight()
					.Padding(FMargin(0.f, 2.f))
					[
						SNew(STextBlock).Text(FText::FromString(Desc))
					];
			}
		}
	}
}

FReply SUploadProgressWidget::HandleForceQuit()
{
	if (OutForceCancelled != nullptr)
	{
		*OutForceCancelled = true;
	}
	CloseParent();
	return FReply::Handled();
}

void SUploadProgressWidget::CloseParent()
{
	if (TSharedPtr<SWindow> Window = ParentWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
