#include "Menu/GYSessionCardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UGYSessionCardWidget::RebuildWidget()
{
	// WBP 파생 없이 쓰일 때만 트리를 코드로 구성 — WBP 가 그린 루트가 있으면 그대로 존중
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		CardButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CardButton"));

		UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardLines"));

		SessionNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SessionNameText"));
		UVerticalBoxSlot* NameSlot = Lines->AddChildToVerticalBox(SessionNameText);
		NameSlot->SetPadding(FMargin(8.f, 6.f, 8.f, 2.f));

		HostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HostText"));
		UVerticalBoxSlot* HostSlot = Lines->AddChildToVerticalBox(HostText);
		HostSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 2.f));

		InfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InfoText"));
		UVerticalBoxSlot* InfoSlot = Lines->AddChildToVerticalBox(InfoText);
		InfoSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 6.f));

		CardButton->AddChild(Lines);
		WidgetTree->RootWidget = CardButton;
	}
	return Super::RebuildWidget();
}

void UGYSessionCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// WBP reparent 케이스: 디자이너가 그린 위젯을 이름으로 채택
	if (SessionNameText == nullptr) SessionNameText = Cast<UTextBlock>(GetWidgetFromName(TEXT("SessionNameText")));
	if (HostText == nullptr) HostText = Cast<UTextBlock>(GetWidgetFromName(TEXT("HostText")));
	if (InfoText == nullptr) InfoText = Cast<UTextBlock>(GetWidgetFromName(TEXT("InfoText")));
	if (CardButton == nullptr) CardButton = Cast<UButton>(GetWidgetFromName(TEXT("CardButton")));
	if (DeleteButton == nullptr) DeleteButton = Cast<UButton>(GetWidgetFromName(TEXT("DeleteButton")));
	if (PlayStateText == nullptr) PlayStateText = Cast<UTextBlock>(GetWidgetFromName(TEXT("PlayStateText")));

	if (CardButton != nullptr && !CardButton->OnClicked.IsAlreadyBound(this, &UGYSessionCardWidget::HandleCardClicked))
	{
		CardButton->OnClicked.AddDynamic(this, &UGYSessionCardWidget::HandleCardClicked);
	}
	if (DeleteButton != nullptr && !DeleteButton->OnClicked.IsAlreadyBound(this, &UGYSessionCardWidget::HandleDeleteClicked))
	{
		DeleteButton->OnClicked.AddDynamic(this, &UGYSessionCardWidget::HandleDeleteClicked);
	}

	ApplyToWidgets();
}

void UGYSessionCardWidget::Setup(const FGYWorldSummary& InWorld)
{
	WorldId = InWorld.Id;
	CachedWorld = InWorld;
	bHasWorld = true;

	// 서버 상태는 유저에게 숨긴다 (세션은 입장 시 시스템이 붙이는 구현 세부) — 만석만 잠금
	bJoinable = InWorld.Status != TEXT("online") || InWorld.PlayerCount < InWorld.MaxPlayers;

	ApplyToWidgets();
}

void UGYSessionCardWidget::ApplyToWidgets()
{
	if (!bHasWorld) return;

	if (SessionNameText != nullptr)
	{
		SessionNameText->SetText(FText::FromString(CachedWorld.Name));
	}
	if (HostText != nullptr)
	{
		HostText->SetText(FText::FromString(FString::Printf(TEXT("호스트 : %s"),
			CachedWorld.OwnerName.IsEmpty() ? TEXT("공용") : *CachedWorld.OwnerName)));
	}
	if (InfoText != nullptr)
	{
		InfoText->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d   %d/%d"),
			CachedWorld.WorldLevel, CachedWorld.PlayerCount, CachedWorld.MaxPlayers)));
	}
	if (CardButton != nullptr)
	{
		CardButton->SetIsEnabled(bJoinEnabledWanted && bJoinable);
	}
	if (DeleteButton != nullptr)
	{
		// 가동 중(online/starting)엔 삭제 불가 — RPC 정책(offline 만)과 일치
		const bool bDeletable = bDeleteVisibleWanted && CachedWorld.Status == TEXT("offline");
		DeleteButton->SetVisibility(bDeletable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (PlayStateText != nullptr)
	{
		// 접속자가 있는 월드만 "플레이 중" — 빈 online(유휴 회수 대기)은 유저 입장에선 꺼진 것과 같다
		const bool bPlaying = CachedWorld.Status == TEXT("online") && CachedWorld.PlayerCount > 0;
		PlayStateText->SetVisibility(bPlaying ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGYSessionCardWidget::SetJoinEnabled(bool bEnabled)
{
	bJoinEnabledWanted = bEnabled;
	if (CardButton != nullptr)
	{
		CardButton->SetIsEnabled(bEnabled && bJoinable);
	}
}

void UGYSessionCardWidget::SetDeleteVisible(bool bVisible)
{
	bDeleteVisibleWanted = bVisible;
	ApplyToWidgets();
}

void UGYSessionCardWidget::HandleCardClicked()
{
	OnJoinRequested.ExecuteIfBound(WorldId);
}

void UGYSessionCardWidget::HandleDeleteClicked()
{
	OnDeleteRequested.ExecuteIfBound(WorldId);
}
