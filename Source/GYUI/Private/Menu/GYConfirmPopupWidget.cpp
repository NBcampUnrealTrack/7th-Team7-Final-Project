#include "Menu/GYConfirmPopupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UGYConfirmPopupWidget::RebuildWidget()
{
	// WBP 파생 없이 쓰일 때만 트리를 코드로 구성 — WBP 가 그린 루트가 있으면 그대로 존중
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ModalRoot"));

		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalDim"));
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Root->AddChild(Dim);
		if (UCanvasPanelSlot* DimSlot = Cast<UCanvasPanelSlot>(Dim->Slot))
		{
			DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			DimSlot->SetOffsets(FMargin(0.f));
			DimSlot->SetZOrder(-1);
		}

		UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalBox"));
		Box->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.05f, 0.95f));
		Box->SetPadding(FMargin(28.f, 22.f));
		Root->AddChild(Box);
		if (UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(Box->Slot))
		{
			BoxSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			BoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			BoxSlot->SetAutoSize(true);
		}

		UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModalLines"));

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 20;
		TitleText->SetFont(TitleFont);
		TitleText->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* TitleSlot = Lines->AddChildToVerticalBox(TitleText);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);

		BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BodyText"));
		BodyText->SetJustification(ETextJustify::Center);
		BodyText->SetAutoWrapText(true);
		UVerticalBoxSlot* BodySlot = Lines->AddChildToVerticalBox(BodyText);
		BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
		BodySlot->SetHorizontalAlignment(HAlign_Center);

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));

		ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
		UTextBlock* ConfirmLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmLabel"));
		ConfirmLabel->SetText(FText::FromString(TEXT("확인")));
		ConfirmButton->AddChild(ConfirmLabel);
		UHorizontalBoxSlot* ConfirmSlot = Buttons->AddChildToHorizontalBox(ConfirmButton);
		ConfirmSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

		CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
		UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelLabel"));
		CancelLabel->SetText(FText::FromString(TEXT("취소")));
		CancelButton->AddChild(CancelLabel);
		Buttons->AddChildToHorizontalBox(CancelButton);

		UVerticalBoxSlot* ButtonsSlot = Lines->AddChildToVerticalBox(Buttons);
		ButtonsSlot->SetHorizontalAlignment(HAlign_Center);

		Box->AddChild(Lines);
		WidgetTree->RootWidget = Root;
	}
	return Super::RebuildWidget();
}

void UGYConfirmPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ConfirmButton != nullptr && !ConfirmButton->OnClicked.IsAlreadyBound(this, &UGYConfirmPopupWidget::HandleConfirmClicked))
	{
		ConfirmButton->OnClicked.AddDynamic(this, &UGYConfirmPopupWidget::HandleConfirmClicked);
	}
	if (CancelButton != nullptr && !CancelButton->OnClicked.IsAlreadyBound(this, &UGYConfirmPopupWidget::HandleCancelClicked))
	{
		CancelButton->OnClicked.AddDynamic(this, &UGYConfirmPopupWidget::HandleCancelClicked);
	}
	ApplyTexts();
}

void UGYConfirmPopupWidget::SetupConfirm(const FText& Title, const FText& Body, FGYOnConfirmed OnConfirm)
{
	CachedTitle = Title;
	CachedBody = Body;
	OnConfirmed = MoveTemp(OnConfirm);
	ApplyTexts();
}

void UGYConfirmPopupWidget::ApplyTexts()
{
	// 빈 캐시로 덮지 않는다 — SetupConfirm 전(디자이너 프리뷰 포함)엔 WBP 텍스트 유지
	if (TitleText != nullptr && !CachedTitle.IsEmpty())
	{
		TitleText->SetText(CachedTitle);
	}
	if (BodyText != nullptr && !CachedBody.IsEmpty())
	{
		BodyText->SetText(CachedBody);
	}
}

void UGYConfirmPopupWidget::HandleConfirmClicked()
{
	FGYOnConfirmed Confirmed = MoveTemp(OnConfirmed);
	RemoveFromParent();
	Confirmed.ExecuteIfBound();
}

void UGYConfirmPopupWidget::HandleCancelClicked()
{
	OnConfirmed.Unbind();
	RemoveFromParent();
}
