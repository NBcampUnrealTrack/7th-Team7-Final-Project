#include "Widget/Loot/GYLootBoxScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Loot/LootBoxActor.h"
#include "Loot/LootTypes.h"
#include "Loot/LootViewerComponent.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"
#include "Widget/Loot/GYLootDropSlotWidget.h"

void UGYLootBoxScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 갱신은 BindToBox 이후의 메시지/콜백에서만. 여기서 Refresh 하면 미바인딩 상태로 자동 닫힘
	if (UWorld* World = GetWorld())
	{
		ListenerHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(
			GYGameplayTags::Message_Loot_BoxStateChanged,
			this,
			&UGYLootBoxScreenWidget::HandleBoxStateChanged);
	}
}

void UGYLootBoxScreenWidget::NativeDestruct()
{
	ReleaseOccupancy();

	if (BoundBox.IsValid())
	{
		BoundBox->OnDestroyed.RemoveDynamic(this, &UGYLootBoxScreenWidget::HandleBoxDestroyed);
	}

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
	const bool bGone = !IsValid(Box);
	// 모두 줍거나 박스가 사라진 경우에만 닫음. 갓 열려 아직 복제 안 된 상태는 빈 채로 대기
	const bool bLootedEmpty = !bGone && Box->IsOpened() && Box->GetPendingDrops().IsEmpty();

	if (bGone || bLootedEmpty)
	{
		SlotContainer->ClearChildren();
		SlotWidgets.Reset();
		OnBoxRefreshed(0);
		DeactivateWidget();
		return;
	}

	const TArray<FLootDrop>& Drops = Box->GetPendingDrops();

	SlotContainer->ClearChildren();
	SlotWidgets.Reset(Drops.Num());

	if (DropSlotWidgetClass != nullptr)
	{
		for (int32 i = 0; i < Drops.Num(); ++i)
		{
			UGYLootDropSlotWidget* SlotWidget = WidgetTree->ConstructWidget<UGYLootDropSlotWidget>(DropSlotWidgetClass);
			if (SlotWidget == nullptr) continue;

			SlotContainer->AddChild(SlotWidget);
			SlotWidget->SetDrop(Box, i, Drops[i]);
			SlotWidgets.Add(SlotWidget);
		}
	}

	OnBoxRefreshed(Drops.Num());
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
