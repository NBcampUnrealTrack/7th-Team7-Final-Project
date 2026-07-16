#include "Menu/GYJoinStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UGYJoinStatusWidget::RebuildWidget()
{
	// WBP 파생 없이 쓰일 때만 트리를 코드로 구성 — WBP 가 그린 루트가 있으면 그대로 존중
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ModalRoot"));

		// 옅은 딤 — 뒤가 비치되 모달에 시선 집중 (뒤 UI 잠금은 bJoinInProgress 가드가 담당)
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalDim"));
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Root->AddChild(Dim);
		if (UCanvasPanelSlot* DimSlot = Cast<UCanvasPanelSlot>(Dim->Slot))
		{
			DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			DimSlot->SetOffsets(FMargin(0.f));
			DimSlot->SetZOrder(-1);
		}

		// 중앙 박스
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

		StatusTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusTitleText"));
		FSlateFontInfo TitleFont = StatusTitleText->GetFont();
		TitleFont.Size = 22;
		StatusTitleText->SetFont(TitleFont);
		StatusTitleText->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* TitleSlot = Lines->AddChildToVerticalBox(StatusTitleText);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);

		StatusBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusBodyText"));
		StatusBodyText->SetJustification(ETextJustify::Center);
		StatusBodyText->SetAutoWrapText(true);
		UVerticalBoxSlot* BodySlot = Lines->AddChildToVerticalBox(StatusBodyText);
		BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
		BodySlot->SetHorizontalAlignment(HAlign_Center);

		CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
		UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelLabel"));
		CancelLabel->SetText(FText::FromString(TEXT("취소")));
		CancelButton->AddChild(CancelLabel);
		UVerticalBoxSlot* CancelSlot = Lines->AddChildToVerticalBox(CancelButton);
		CancelSlot->SetHorizontalAlignment(HAlign_Center);

		Box->AddChild(Lines);
		WidgetTree->RootWidget = Root;
	}
	return Super::RebuildWidget();
}

void UGYJoinStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CancelButton != nullptr && !CancelButton->OnClicked.IsAlreadyBound(this, &UGYJoinStatusWidget::HandleCancelClicked))
	{
		CancelButton->OnClicked.AddDynamic(this, &UGYJoinStatusWidget::HandleCancelClicked);
	}
}

void UGYJoinStatusWidget::SetPhase(int64 WorldId, EGYJoinWorldPhase Phase)
{
	FText Title;
	FText Body;
	bool bCanCancel = true;

	switch (Phase)
	{
	case EGYJoinWorldPhase::Requested:
		Title = RequestedTitle;
		Body = RequestedBody;
		break;

	case EGYJoinWorldPhase::Queued:
		Title = QueuedTitle;
		Body = QueuedBody;
		break;

	case EGYJoinWorldPhase::Starting:
		Title = StartingTitle;
		Body = StartingBody;
		break;

	case EGYJoinWorldPhase::Online:
		Title = OnlineTitle;
		Body = OnlineBody;
		bCanCancel = false; // 접속 개시 후엔 여행이 진행돼 취소 불가
		break;

	default:
		break;
	}

	if (StatusTitleText != nullptr)
	{
		StatusTitleText->SetText(Title);
	}
	if (StatusBodyText != nullptr)
	{
		StatusBodyText->SetText(Body);
	}
	if (CancelButton != nullptr)
	{
		CancelButton->SetIsEnabled(bCanCancel);
	}
}

void UGYJoinStatusWidget::HandleCancelClicked()
{
	UGameInstance* GameInstance = GetGameInstance();
	UGYAccountSubsystem* Account = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYAccountSubsystem>() : nullptr;
	if (Account != nullptr)
	{
		Account->CancelJoin(); // Failed 페이즈 통지 → 세션 메뉴가 모달 제거 + 카드 잠금 해제
	}
}
