#include "Menu/GYSessionMenuWidget.h"

#include "Menu/GYSessionCardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "HAL/IConsoleManager.h"
#include "Logging/GYLogManager.h"
#include "Widget/Common/GYMessagePopupWidget.h"

namespace
{
	// dev: 팀원 메뉴 연동 전 단독 확인용 — 뷰포트에 세션 패널을 직접 띄운다.
	// WBP(디자인판)가 있으면 그걸, 없으면 C++ 폴백 구성으로
	void SessionMenuCmd(const TArray<FString>& Args, UWorld* World)
	{
		if (!IsValid(World)) return;
		APlayerController* PC = World->GetFirstPlayerController();
		if (!IsValid(PC)) return;

		UClass* MenuClass = LoadClass<UGYSessionMenuWidget>(nullptr,
			TEXT("/Game/GY/UI/MainMenu/Session/WBP_SessionMenu.WBP_SessionMenu_C"));
		if (MenuClass == nullptr)
		{
			MenuClass = UGYSessionMenuWidget::StaticClass();
		}

		UGYSessionMenuWidget* Menu = CreateWidget<UGYSessionMenuWidget>(PC, MenuClass);
		if (Menu != nullptr)
		{
			Menu->AddToViewport(10);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GYSessionMenuCommand(
		TEXT("gy.UI.SessionMenu"),
		TEXT("Open the session panel widget directly (dev)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SessionMenuCmd));
}

UGYSessionMenuWidget::UGYSessionMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EGYWidgetInputMode::Menu;
	SessionCardClass = UGYSessionCardWidget::StaticClass();
}

TSharedRef<SWidget> UGYSessionMenuWidget::RebuildWidget()
{
	// 순수 C++ 생성(WBP 없음): 슬레이트 트리 구축 전에 루트가 있어야 렌더된다
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
	{
		BuildFallbackPanel();
	}
	return Super::RebuildWidget();
}

void UGYSessionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveOrBuildWidgets();
	SetMode(0);

	UGYAccountSubsystem* Account = ResolveAccount();
	if (Account != nullptr)
	{
		JoinPhaseHandle = Account->OnJoinWorldPhase.AddUObject(this, &UGYSessionMenuWidget::OnJoinPhase);

		if (Account->IsLoggedIn())
		{
			RefreshSessions();
		}
		else
		{
			SetStatus(TEXT("로그인 중..."));
			AccountReadyHandle = Account->OnAccountReady.AddUObject(this, &UGYSessionMenuWidget::OnAccountReady);
		}
	}
	else
	{
		SetStatus(TEXT("계정 시스템 없음"));
	}
}

void UGYSessionMenuWidget::NativeDestruct()
{
	if (UGYAccountSubsystem* Account = ResolveAccount())
	{
		Account->OnAccountReady.Remove(AccountReadyHandle);
		Account->OnJoinWorldPhase.Remove(JoinPhaseHandle);
	}
	Super::NativeDestruct();
}

void UGYSessionMenuWidget::ResolveOrBuildWidgets()
{
	// WBP(reparent) 가 그린 위젯을 이름으로 채택 — 목록 컨테이너가 없으면 코드 구성 폴백
	ModeSwitcher = Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("ModeSwitcher")));
	MySessionList = Cast<UPanelWidget>(GetWidgetFromName(TEXT("MySessionList")));
	AllSessionList = Cast<UPanelWidget>(GetWidgetFromName(TEXT("AllSessionList")));
	NameInputBox = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("NameInputBox")));
	StatusText = Cast<UTextBlock>(GetWidgetFromName(TEXT("StatusText")));

	// 폴백 구성 시 이미 바인딩된 버튼은 건너뜀 (이중 클릭 핸들 방지)
	if (UButton* CreateOpenButton = Cast<UButton>(GetWidgetFromName(TEXT("CreateOpenButton"))))
	{
		if (!CreateOpenButton->OnClicked.IsAlreadyBound(this, &UGYSessionMenuWidget::HandleCreateOpenClicked))
		{
			CreateOpenButton->OnClicked.AddDynamic(this, &UGYSessionMenuWidget::HandleCreateOpenClicked);
		}
	}
	if (UButton* RefreshButton = Cast<UButton>(GetWidgetFromName(TEXT("RefreshButton"))))
	{
		if (!RefreshButton->OnClicked.IsAlreadyBound(this, &UGYSessionMenuWidget::HandleRefreshClicked))
		{
			RefreshButton->OnClicked.AddDynamic(this, &UGYSessionMenuWidget::HandleRefreshClicked);
		}
	}
	if (UButton* CreateConfirmButton = Cast<UButton>(GetWidgetFromName(TEXT("CreateConfirmButton"))))
	{
		if (!CreateConfirmButton->OnClicked.IsAlreadyBound(this, &UGYSessionMenuWidget::HandleCreateConfirmClicked))
		{
			CreateConfirmButton->OnClicked.AddDynamic(this, &UGYSessionMenuWidget::HandleCreateConfirmClicked);
		}
	}
	if (UButton* CreateCancelButton = Cast<UButton>(GetWidgetFromName(TEXT("CreateCancelButton"))))
	{
		if (!CreateCancelButton->OnClicked.IsAlreadyBound(this, &UGYSessionMenuWidget::HandleCreateCancelClicked))
		{
			CreateCancelButton->OnClicked.AddDynamic(this, &UGYSessionMenuWidget::HandleCreateCancelClicked);
		}
	}

	// WBP 루트는 있는데 목록 컨테이너가 없는 껍데기 케이스 — 런타임 삽입 (기존 패널에 AddChild 는 즉시 렌더 반영)
	if (AllSessionList == nullptr)
	{
		BuildFallbackPanel();
	}
}

void UGYSessionMenuWidget::BuildFallbackPanel()
{
	UPanelWidget* RootPanel = Cast<UPanelWidget>(GetRootWidget());
	if (WidgetTree == nullptr) return;

	// 위젯이 통째로 코드 생성일 수도(루트 없음), WBP 껍데기에 얹힐 수도 있다
	if (RootPanel == nullptr && GetRootWidget() == nullptr)
	{
		RootPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SessionRoot"));
		WidgetTree->RootWidget = RootPanel;
	}
	if (RootPanel == nullptr) return;

	ModeSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("ModeSwitcher"));

	// ── 페이지 0: 목록 ──
	UVerticalBox* ListPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ListPage"));

	UButton* CreateOpenButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateOpenButton"));
	UTextBlock* CreateOpenLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreateOpenLabel"));
	CreateOpenLabel->SetText(FText::FromString(TEXT("월드 생성")));
	CreateOpenButton->AddChild(CreateOpenLabel);
	CreateOpenButton->OnClicked.AddDynamic(this, &UGYSessionMenuWidget::HandleCreateOpenClicked);
	ListPage->AddChildToVerticalBox(CreateOpenButton);

	// 섹션별 고정 영역 + 개별 스크롤 — 세로 절반씩
	UTextBlock* MyHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MySessionHeader"));
	MyHeader->SetText(FText::FromString(TEXT("참가 중인 월드")));
	UVerticalBoxSlot* MyHeaderSlot = ListPage->AddChildToVerticalBox(MyHeader);
	MyHeaderSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 4.f));

	UScrollBox* MyScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MySessionScroll"));
	UVerticalBox* MyList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MySessionList"));
	MyScroll->AddChild(MyList);
	MySessionList = MyList;
	UVerticalBoxSlot* MyScrollSlot = ListPage->AddChildToVerticalBox(MyScroll);
	MyScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UTextBlock* AllHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AllSessionHeader"));
	AllHeader->SetText(FText::FromString(TEXT("모든 월드")));
	UVerticalBoxSlot* AllHeaderSlot = ListPage->AddChildToVerticalBox(AllHeader);
	AllHeaderSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 4.f));

	UScrollBox* AllScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("AllSessionScroll"));
	UVerticalBox* AllList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AllSessionList"));
	AllScroll->AddChild(AllList);
	AllSessionList = AllList;
	UVerticalBoxSlot* AllScrollSlot = ListPage->AddChildToVerticalBox(AllScroll);
	AllScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UButton* RefreshButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RefreshButton"));
	UTextBlock* RefreshLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RefreshLabel"));
	RefreshLabel->SetText(FText::FromString(TEXT("새로고침")));
	RefreshButton->AddChild(RefreshLabel);
	RefreshButton->OnClicked.AddDynamic(this, &UGYSessionMenuWidget::HandleRefreshClicked);
	ListPage->AddChildToVerticalBox(RefreshButton);

	ModeSwitcher->AddChild(ListPage);

	// ── 페이지 1: 생성 ──
	UVerticalBox* CreatePage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreatePage"));

	NameInputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NameInputBox"));
	NameInputBox->SetHintText(FText::FromString(TEXT("월드 이름 : 입력하세여~")));
	CreatePage->AddChildToVerticalBox(NameInputBox);

	UHorizontalBox* CreateButtons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CreateButtons"));
	UButton* ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateConfirmButton"));
	UTextBlock* ConfirmLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmLabel"));
	ConfirmLabel->SetText(FText::FromString(TEXT("생성")));
	ConfirmButton->AddChild(ConfirmLabel);
	ConfirmButton->OnClicked.AddDynamic(this, &UGYSessionMenuWidget::HandleCreateConfirmClicked);
	CreateButtons->AddChildToHorizontalBox(ConfirmButton);

	UButton* CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateCancelButton"));
	UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelLabel"));
	CancelLabel->SetText(FText::FromString(TEXT("취소")));
	CancelButton->AddChild(CancelLabel);
	CancelButton->OnClicked.AddDynamic(this, &UGYSessionMenuWidget::HandleCreateCancelClicked);
	CreateButtons->AddChildToHorizontalBox(CancelButton);
	CreatePage->AddChildToVerticalBox(CreateButtons);

	ModeSwitcher->AddChild(CreatePage);

	// ── 공통 프레임: 스위처 + 상태 줄 ──
	UVerticalBox* Frame = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SessionFrame"));
	UVerticalBoxSlot* SwitcherSlot = Frame->AddChildToVerticalBox(ModeSwitcher);
	SwitcherSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	Frame->AddChildToVerticalBox(StatusText);

	RootPanel->AddChild(Frame);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Frame->Slot))
	{
		// 목업: 우측 세로 패널
		CanvasSlot->SetAnchors(FAnchors(0.72f, 0.05f, 0.98f, 0.95f));
		CanvasSlot->SetOffsets(FMargin(0.f));
	}
}

void UGYSessionMenuWidget::SetMode(int32 SwitcherIndex)
{
	if (ModeSwitcher != nullptr)
	{
		ModeSwitcher->SetActiveWidgetIndex(SwitcherIndex);
	}
}

void UGYSessionMenuWidget::RefreshSessions()
{
	UGYAccountSubsystem* Account = ResolveAccount();
	if (Account == nullptr || !Account->IsLoggedIn()) return;

	SetStatus(TEXT("목록 불러오는 중..."));
	Account->ListWorlds(FGYOnWorldList::CreateUObject(this, &UGYSessionMenuWidget::OnWorldList));
}

void UGYSessionMenuWidget::OnWorldList(bool bSuccess, const TArray<FGYWorldSummary>& Worlds)
{
	if (AllSessionList == nullptr) return;

	if (!bSuccess)
	{
		SetStatus(TEXT("목록 조회 실패 - 새로고침으로 재시도"));
		return;
	}

	if (MySessionList != nullptr)
	{
		MySessionList->ClearChildren();
	}
	AllSessionList->ClearChildren();
	Cards.Reset();

	UGYAccountSubsystem* Account = ResolveAccount();
	const FString MyAccountId = Account != nullptr ? Account->GetAccountId() : FString();

	int32 MyCount = 0;
	for (const FGYWorldSummary& World : Worlds)
	{
		const bool bMine = World.bParticipant || (!World.OwnerAccountId.IsEmpty() && World.OwnerAccountId == MyAccountId);
		UPanelWidget* Container = bMine && MySessionList != nullptr ? MySessionList.Get() : AllSessionList.Get();
		if (AddCard(Container, World) != nullptr && bMine)
		{
			MyCount++;
		}
	}

	SetStatus(Worlds.Num() > 0 ? FString() : TEXT("월드가 없습니다 - 생성해 보세요"));
	GY_LOG(Network, KDY, "Session list refreshed (mine=%d all=%d)", MyCount, Worlds.Num());
}

UGYSessionCardWidget* UGYSessionMenuWidget::AddCard(UPanelWidget* Container, const FGYWorldSummary& World)
{
	if (Container == nullptr) return nullptr;

	UGYSessionCardWidget* Card = CreateWidget<UGYSessionCardWidget>(this, SessionCardClass != nullptr ? *SessionCardClass : UGYSessionCardWidget::StaticClass());
	if (Card == nullptr) return nullptr;

	Card->Setup(World);
	Card->OnJoinRequested.BindUObject(this, &UGYSessionMenuWidget::JoinWorld);
	Card->SetJoinEnabled(!bJoinInProgress);
	Container->AddChild(Card);
	Cards.Add(Card);
	return Card;
}

void UGYSessionMenuWidget::JoinWorld(int64 WorldId)
{
	UGYAccountSubsystem* Account = ResolveAccount();
	if (Account == nullptr || bJoinInProgress) return;

	Account->JoinWorld(WorldId);
}

void UGYSessionMenuWidget::OnJoinPhase(int64 WorldId, EGYJoinWorldPhase Phase)
{
	switch (Phase)
	{
	case EGYJoinWorldPhase::Requested:
		bJoinInProgress = true;
		SetCardsEnabled(false);
		SetStatus(FString::Printf(TEXT("월드 %lld 준비 중..."), WorldId));
		break;

	case EGYJoinWorldPhase::Starting:
		SetStatus(FString::Printf(TEXT("월드 %lld 준비 중... (최초 입장은 몇 분 걸릴 수 있어요)"), WorldId));
		break;

	case EGYJoinWorldPhase::Online:
		SetStatus(FString::Printf(TEXT("월드 %lld 접속 중..."), WorldId));
		break;

	case EGYJoinWorldPhase::Failed:
		bJoinInProgress = false;
		SetCardsEnabled(true);
		SetStatus(TEXT("입장 실패 - 새로고침 후 다시 시도하세요"));
		UGYMessagePopupWidget::ShowNotice(
			this,
			NSLOCTEXT("GYUI", "Join_Failed_Title", "입장 실패"),
			NSLOCTEXT("GYUI", "Join_Failed_Message",
					  "세션에 입장하지 못했습니다.\n잠시 후 다시 시도해주세요."));
		break;
	}
}

void UGYSessionMenuWidget::OnAccountReady(bool bSuccess)
{
	if (bSuccess)
	{
		SetStatus(FString());
		RefreshSessions();
	}
	else
	{
		SetStatus(TEXT("로그인 실패 - 백엔드 연결을 확인하세요"));
	}
}

void UGYSessionMenuWidget::HandleRefreshClicked()
{
	if (!bJoinInProgress)
	{
		RefreshSessions();
	}
}

void UGYSessionMenuWidget::HandleCreateOpenClicked()
{
	if (!bJoinInProgress)
	{
		SetMode(1);
		SetStatus(FString());
	}
}

void UGYSessionMenuWidget::HandleCreateCancelClicked()
{
	SetMode(0);
	SetStatus(FString());
}

void UGYSessionMenuWidget::HandleCreateConfirmClicked()
{
	UGYAccountSubsystem* Account = ResolveAccount();
	if (Account == nullptr || NameInputBox == nullptr || bJoinInProgress) return;

	const FString WorldName = NameInputBox->GetText().ToString().TrimStartAndEnd().Left(40);
	if (WorldName.IsEmpty())
	{
		SetStatus(TEXT("월드 이름을 입력하세요"));
		return;
	}

	SetStatus(TEXT("월드 생성 중..."));
	Account->CreateWorld(WorldName, FGYOnWorldOp::CreateWeakLambda(this,
		[this](bool bSuccess, int64 WorldId)
		{
			if (bSuccess)
			{
				if (NameInputBox != nullptr)
				{
					NameInputBox->SetText(FText::GetEmpty());
				}
				SetMode(0);
				RefreshSessions();
			}
			else
			{
				SetStatus(TEXT("생성 실패 (캐릭터당 월드 수 제한 확인)"));
			}
		}));
}

void UGYSessionMenuWidget::SetStatus(const FString& Message)
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}

void UGYSessionMenuWidget::SetCardsEnabled(bool bEnabled)
{
	for (UGYSessionCardWidget* Card : Cards)
	{
		if (Card != nullptr)
		{
			Card->SetJoinEnabled(bEnabled);
		}
	}
}

UGYAccountSubsystem* UGYSessionMenuWidget::ResolveAccount() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYAccountSubsystem>() : nullptr;
}
