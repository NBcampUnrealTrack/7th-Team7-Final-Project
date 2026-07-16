#include "Widget/Loot/GYLootBoxScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Loot/LootBoxActor.h"
#include "Loot/LootTypes.h"
#include "Loot/LootViewerComponent.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"
#include "Widget/Loot/GYLootDropSlotWidget.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGYLootBoxScreenWidget::UGYLootBoxScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 슬롯 마우스 클릭 + 위젯이 키 포커스를 받도록 메뉴 입력 모드 고정
	InputMode = EGYWidgetInputMode::Menu;
}

void UGYLootBoxScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// F/ESC 키 입력을 NativeOnKeyDown에서 받기 위해 포커스 가능하도록
	SetIsFocusable(true);

	// 갱신은 BindToBox 이후의 메시지/콜백에서만. 여기서 Refresh 하면 미바인딩 상태로 자동 닫힘
	// 프리뷰/썸네일 등 GameInstance 없는 월드에서는 서브시스템이 없으므로 Get() 어설션 회피
	UWorld* World = GetWorld();
	if (World != nullptr && UGameplayMessageSubsystem::HasInstance(World))
	{
		ListenerHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(
			GYGameplayTags::Message_Loot_BoxStateChanged,
			this,
			&UGYLootBoxScreenWidget::HandleBoxStateChanged);
	}

	if (Btn_Close)
	{
		Btn_Close->OnClicked.AddUniqueDynamic(this, &UGYLootBoxScreenWidget::HandleCloseClicked);
	}
}

void UGYLootBoxScreenWidget::NativeDestruct()
{
	// 점유 해제 시 빈 상자가 동기 파괴될 수 있으므로 콜백부터 끊고 해제
	if (BoundBox.IsValid())
	{
		BoundBox->OnDestroyed.RemoveDynamic(this, &UGYLootBoxScreenWidget::HandleBoxDestroyed);
	}

	ReleaseOccupancy();

	if (ListenerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayMessageSubsystem::Get(World).UnregisterListener(ListenerHandle);
		}
		ListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::NativeDestruct();
}

void UGYLootBoxScreenWidget::BindToBox(ALootBoxActor* Box)
{
	if (BoundBox.Get() == Box)
	{
		Refresh();
		return;
	}

	if (BoundBox.IsValid())
	{
		BoundBox->OnDestroyed.RemoveDynamic(this, &UGYLootBoxScreenWidget::HandleBoxDestroyed);
	}

	BoundBox = Box;

	if (IsValid(Box))
	{
		Box->OnDestroyed.AddDynamic(this, &UGYLootBoxScreenWidget::HandleBoxDestroyed);
	}

	Refresh();
}

void UGYLootBoxScreenWidget::Refresh()
{
	if (SlotContainer == nullptr) return;

	ALootBoxActor* Box = BoundBox.Get();
	// 상자가 파괴된 경우에만 자동으로 닫음. 전부 주워 비어도 직접 닫기 전까지 빈 그리드로 유지
	if (!IsValid(Box))
	{
		SlotContainer->ClearChildren();
		SlotWidgets.Reset();
		OnBoxRefreshed(0);
		DeactivateWidget();
		return;
	}

	EnsureSlots();

	const TArray<FLootDrop>& Drops = Box->GetPendingDrops();

	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		UGYLootDropSlotWidget* SlotWidget = SlotWidgets[i];
		if (!IsValid(SlotWidget)) continue;

		if (Drops.IsValidIndex(i))
		{
			SlotWidget->SetDrop(Box, i, Drops[i]);
		}
		else
		{
			SlotWidget->SetEmpty();
		}
	}

	OnBoxRefreshed(Drops.Num());
}

void UGYLootBoxScreenWidget::EnsureSlots()
{
	if (SlotContainer == nullptr || DropSlotWidgetClass == nullptr) return;

	ALootBoxActor* Box = BoundBox.Get();
	const int32 DropCount = IsValid(Box) ? Box->GetPendingDrops().Num() : 0;
	const int32 DesiredCount = FMath::Max(GridSlotCount, DropCount);

	// 한 번 만든 뒤로는 늘리기만 — 줍는 도중 그리드가 줄어 깜빡이지 않도록
	if (SlotWidgets.Num() >= DesiredCount) return;

	SlotContainer->ClearChildren();
	SlotWidgets.Reset(DesiredCount);

	for (int32 i = 0; i < DesiredCount; ++i)
	{
		UGYLootDropSlotWidget* SlotWidget = WidgetTree->ConstructWidget<UGYLootDropSlotWidget>(DropSlotWidgetClass);
		if (SlotWidget == nullptr) continue;

		SlotContainer->AddChild(SlotWidget);
		SlotWidgets.Add(SlotWidget);
	}
}

void UGYLootBoxScreenWidget::ReleaseOccupancy()
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	if (!IsValid(PS)) return;

	ULootViewerComponent* Viewer = PS->GetLootViewerComponent();
	if (!IsValid(Viewer)) return;

	Viewer->Server_CloseLootBox(BoundBox.Get());
}

void UGYLootBoxScreenWidget::HandleBoxStateChanged(FGameplayTag, const FGYLootBoxStateMessage& Msg)
{
	if (Msg.Box.Get() != BoundBox.Get()) return;
	Refresh();
}

void UGYLootBoxScreenWidget::HandleBoxDestroyed(AActor*)
{
	Refresh();
}

void UGYLootBoxScreenWidget::HandleCloseClicked()
{
	DeactivateWidget();
}

void UGYLootBoxScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	FocusRetryCount = 0;
	ScheduleFocusAttempt();
}

void UGYLootBoxScreenWidget::ScheduleFocusAttempt()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;

	TWeakObjectPtr<UGYLootBoxScreenWidget> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick([WeakThis]()
	{
		if (UGYLootBoxScreenWidget* Widget = WeakThis.Get())
		{
			Widget->EnsureKeyboardFocus();
		}
	});
}

void UGYLootBoxScreenWidget::EnsureKeyboardFocus()
{
	if (!IsActivated()) return;

	if (!HasAnyUserFocus() && !HasFocusedDescendants())
	{
		SetFocus();
	}

	if (++FocusRetryCount < MaxFocusRetries)
	{
		ScheduleFocusAttempt();
	}
}

FReply UGYLootBoxScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::F)
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

UWidget* UGYLootBoxScreenWidget::NativeGetDesiredFocusTarget() const
{
	return const_cast<UGYLootBoxScreenWidget*>(this);
}
