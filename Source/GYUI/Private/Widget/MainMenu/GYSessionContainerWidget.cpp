#include "Widget/MainMenu/GYSessionContainerWidget.h"

#include "Account/GYAccountSubsystem.h"
#include "Menu/GYConfirmPopupWidget.h"
#include "Menu/GYJoinStatusWidget.h"
#include "Menu/GYSessionCardWidget.h"
#include "Widget/Common/GYMessagePopupWidget.h"
#include "Widget/MainMenu/GYSessionCreateWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "HAL/IConsoleManager.h"
#include "Logging/GYLogManager.h"

namespace
{
	// dev: 메인메뉴 연동 전 단독 확인용 — 세션 패널을 토글한다 (재실행 시 새 인스턴스 중첩 금지)
	void SessionMenuCmd(const TArray<FString>& Args, UWorld* World)
	{
		if (!IsValid(World)) return;
		APlayerController* PC = World->GetFirstPlayerController();
		if (!IsValid(PC)) return;

		TArray<UUserWidget*> Existing;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Existing, UGYSessionContainerWidget::StaticClass(), false);
		if (Existing.Num() > 0)
		{
			CastChecked<UGYSessionContainerWidget>(Existing[0])->TogglePanel();
			return;
		}

		UClass* PanelClass = LoadClass<UGYSessionContainerWidget>(nullptr,
			TEXT("/Game/GY/UI/MainMenu/Session/WBP_SessionContainer.WBP_SessionContainer_C"));
		if (PanelClass == nullptr)
		{
			GY_WARN(Network, KDY, "gy.UI.SessionMenu: WBP_SessionContainer not found");
			return;
		}

		UGYSessionContainerWidget* Panel = CreateWidget<UGYSessionContainerWidget>(PC, PanelClass);
		if (Panel != nullptr)
		{
			Panel->AddToViewport(10);
			Panel->OpenPanel();
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GYSessionMenuCommand(
		TEXT("gy.UI.SessionMenu"),
		TEXT("Open the session panel widget directly (dev)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SessionMenuCmd));
}

void UGYSessionContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_CreateSession)
		Button_CreateSession->OnClicked.AddDynamic(this, &UGYSessionContainerWidget::HandleCreateSessionClicked);

	if (Button_Back)
		Button_Back->OnClicked.AddDynamic(this, &UGYSessionContainerWidget::HandleBackClicked);

	// 디자이너 프리뷰용 카드 제거 — 실제 목록은 조회 응답이 채운다
	if (SessionScrollBox != nullptr)
	{
		SessionScrollBox->ClearChildren();
	}

	if (UGYWorldSessionSubsystem* Session = ResolveSession())
	{
		JoinPhaseHandle = Session->OnJoinWorldPhase.AddUObject(this, &UGYSessionContainerWidget::OnJoinPhase);
	}

	UGYAccountSubsystem* Account = ResolveAccount();
	if (Account != nullptr)
	{
		if (Account->IsLoggedIn())
		{
			RefreshSessions();
		}
		else
		{
			AccountReadyHandle = Account->OnAccountReady.AddUObject(this, &UGYSessionContainerWidget::OnAccountReady);
		}
	}
}

void UGYSessionContainerWidget::NativeDestruct()
{
	if (UGYAccountSubsystem* Account = ResolveAccount())
	{
		Account->OnAccountReady.Remove(AccountReadyHandle);
	}
	if (UGYWorldSessionSubsystem* Session = ResolveSession())
	{
		Session->OnJoinWorldPhase.Remove(JoinPhaseHandle);
	}
	Super::NativeDestruct();
}

void UGYSessionContainerWidget::HandleCreateSessionClicked()
{
	if (bJoinInProgress) return;

	// 구독자(메인메뉴)가 있으면 생성 패널은 그쪽 소유 — 여기서 또 열면 두 장이 겹친다
	if (OnCreateSessionRequested.IsBound())
	{
		OnCreateSessionRequested.Broadcast();
		return;
	}

	// 단독 실행(콘솔 gy.UI.SessionMenu) 폴백 — 자체 생성 패널, 성공 시 목록 갱신을 구독
	if (CreatePanelClass != nullptr)
	{
		if (CreatePanel == nullptr)
		{
			CreatePanel = CreateWidget<UGYSessionCreateWidget>(GetOwningPlayer(), CreatePanelClass);
			if (CreatePanel != nullptr)
			{
				CreatePanel->AddToViewport(15);
				CreatePanel->OnSessionCreated.AddDynamic(this, &UGYSessionContainerWidget::RefreshSessions);
			}
		}
		if (CreatePanel != nullptr)
		{
			CreatePanel->OpenPanel();
		}
	}
}

void UGYSessionContainerWidget::HandleBackClicked()
{
	ClosePanel();
}

void UGYSessionContainerWidget::RefreshSessions()
{
	UGYWorldSessionSubsystem* Session = ResolveSession();
	if (Session == nullptr) return;

	Session->ListWorlds(FGYOnWorldList::CreateUObject(this, &UGYSessionContainerWidget::OnWorldList));
}

void UGYSessionContainerWidget::OnWorldList(bool bSuccess, const TArray<FGYWorldSummary>& Worlds)
{
	if (!bSuccess || SessionScrollBox == nullptr) return;

	SessionScrollBox->ClearChildren();
	Cards.Reset();

	UGYAccountSubsystem* Account = ResolveAccount();
	const FString MyAccountId = Account != nullptr ? Account->GetAccountId() : FString();

	TArray<const FGYWorldSummary*> Mine;
	TArray<const FGYWorldSummary*> Others;
	for (const FGYWorldSummary& World : Worlds)
	{
		const bool bMine = World.bParticipant || (!World.OwnerAccountId.IsEmpty() && World.OwnerAccountId == MyAccountId);
		(bMine ? Mine : Others).Add(&World);
	}

	AddSectionHeader(TEXT("참가 중인 월드"));
	for (const FGYWorldSummary* World : Mine)
	{
		// 삭제는 참여가 아니라 "소유" 기준 — 남의 월드에 참가만 한 카드엔 안 보인다
		AddCard(*World, !World->OwnerAccountId.IsEmpty() && World->OwnerAccountId == MyAccountId);
	}

	AddSectionHeader(TEXT("모든 월드"));
	for (const FGYWorldSummary* World : Others)
	{
		AddCard(*World, false);
	}
}

void UGYSessionContainerWidget::AddSectionHeader(const FString& Label)
{
	UTextBlock* Header = NewObject<UTextBlock>(this);
	Header->SetText(FText::FromString(Label));
	FSlateFontInfo Font = Header->GetFont();
	Font.Size = 14;
	Header->SetFont(Font);
	if (UScrollBoxSlot* HeaderSlot = Cast<UScrollBoxSlot>(SessionScrollBox->AddChild(Header)))
	{
		HeaderSlot->SetPadding(FMargin(4.f, 10.f, 4.f, 4.f));
	}
}

void UGYSessionContainerWidget::AddCard(const FGYWorldSummary& World, bool bMine)
{
	UGYSessionCardWidget* Card = CreateWidget<UGYSessionCardWidget>(this,
		SessionCardClass != nullptr ? *SessionCardClass : UGYSessionCardWidget::StaticClass());
	if (Card == nullptr) return;

	Card->Setup(World);
	Card->OnJoinRequested.BindUObject(this, &UGYSessionContainerWidget::JoinWorld);
	Card->OnDeleteRequested.BindUObject(this, &UGYSessionContainerWidget::RequestDeleteWorld);
	Card->SetJoinEnabled(!bJoinInProgress);
	Card->SetDeleteVisible(bMine);
	SessionScrollBox->AddChild(Card);
	Cards.Add(Card);
}

void UGYSessionContainerWidget::RequestDeleteWorld(int64 WorldId)
{
	if (bJoinInProgress) return;

	const TObjectPtr<UGYSessionCardWidget>* Found = Cards.FindByPredicate(
		[WorldId](const TObjectPtr<UGYSessionCardWidget>& Card) { return IsValid(Card) && Card->GetWorldId() == WorldId; });
	const FString WorldName = Found != nullptr ? (*Found)->GetWorldName() : FString();

	UGYConfirmPopupWidget* Popup = CreateWidget<UGYConfirmPopupWidget>(GetOwningPlayer(),
		ConfirmPopupClass != nullptr ? *ConfirmPopupClass : UGYConfirmPopupWidget::StaticClass());
	if (Popup == nullptr) return;

	Popup->SetupConfirm(
		FText::FromString(TEXT("월드 삭제")),
		FText::FromString(FString::Printf(TEXT("'%s' 월드를 정말 삭제하시겠습니까?"), *WorldName)),
		FGYOnConfirmed::CreateUObject(this, &UGYSessionContainerWidget::ConfirmDeleteWorld, WorldId));
	Popup->AddToViewport(20);
}

void UGYSessionContainerWidget::ConfirmDeleteWorld(int64 WorldId)
{
	UGYWorldSessionSubsystem* Session = ResolveSession();
	if (Session == nullptr) return;

	Session->DeleteWorld(WorldId, FGYOnWorldOp::CreateWeakLambda(this,
		[this](bool bSuccess, int64 /*WorldId*/)
		{
			if (bSuccess)
			{
				RefreshSessions();
			}
		}));
}

void UGYSessionContainerWidget::JoinWorld(int64 WorldId)
{
	UGYWorldSessionSubsystem* Session = ResolveSession();
	if (Session == nullptr || bJoinInProgress) return;

	Session->JoinWorld(WorldId);
}

void UGYSessionContainerWidget::OnJoinPhase(int64 WorldId, EGYJoinWorldPhase Phase)
{
	// 종료 페이즈: 모달 제거 + 카드 잠금 해제. 실패 안내는 진짜 실패만 (사용자 취소엔 생략)
	if (Phase == EGYJoinWorldPhase::Failed || Phase == EGYJoinWorldPhase::Cancelled)
	{
		bJoinInProgress = false;
		SetCardsEnabled(true);
		if (JoinStatusModal != nullptr)
		{
			JoinStatusModal->RemoveFromParent();
			JoinStatusModal = nullptr;
		}
		if (Phase == EGYJoinWorldPhase::Failed)
		{
			UGYMessagePopupWidget::ShowNotice(
				this,
				NSLOCTEXT("GYUI", "Join_Failed_Title", "입장 실패"),
				NSLOCTEXT("GYUI", "Join_Failed_Message",
						  "세션에 입장하지 못했습니다.\n잠시 후 다시 시도해주세요."));
		}
		return;
	}

	if (Phase == EGYJoinWorldPhase::Requested)
	{
		bJoinInProgress = true;
		SetCardsEnabled(false);
	}

	if (JoinStatusModal == nullptr)
	{
		JoinStatusModal = CreateWidget<UGYJoinStatusWidget>(GetOwningPlayer(),
			JoinStatusClass != nullptr ? *JoinStatusClass : UGYJoinStatusWidget::StaticClass());
		if (JoinStatusModal != nullptr)
		{
			JoinStatusModal->AddToViewport(20);
		}
	}
	if (JoinStatusModal != nullptr)
	{
		JoinStatusModal->SetPhase(WorldId, Phase);
	}
	// Online 이후는 ClientTravel 로 월드가 통째로 바뀌며 모달도 함께 정리된다
}

void UGYSessionContainerWidget::OnAccountReady(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshSessions();
	}
}

void UGYSessionContainerWidget::SetCardsEnabled(bool bEnabled)
{
	for (UGYSessionCardWidget* Card : Cards)
	{
		if (Card != nullptr)
		{
			Card->SetJoinEnabled(bEnabled);
		}
	}
}

UGYAccountSubsystem* UGYSessionContainerWidget::ResolveAccount() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYAccountSubsystem>() : nullptr;
}

UGYWorldSessionSubsystem* UGYSessionContainerWidget::ResolveSession() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYWorldSessionSubsystem>() : nullptr;
}
