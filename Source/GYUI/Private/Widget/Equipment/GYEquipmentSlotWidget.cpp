#include "Widget/Equipment/GYEquipmentSlotWidget.h"

#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Equipment/EquipmentLoadoutComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryEntry.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"
#include "Widget/Equipment/GYEquipmentPanelWidget.h"

void UGYEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		ListenerHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(
			GYGameplayTags::Message_Equipment_LoadoutSlotChanged,
			this,
			&UGYEquipmentSlotWidget::HandleLoadoutChanged);
	}

	Refresh();
}

void UGYEquipmentSlotWidget::NativeDestruct()
{
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

UEquipmentLoadoutComponent* UGYEquipmentSlotWidget::GetLoadout() const
{
	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	return IsValid(PS) ? PS->GetEquipmentLoadoutComponent() : nullptr;
}

void UGYEquipmentSlotWidget::HandleLoadoutChanged(FGameplayTag, const FGYEquipSlotMessage& Message)
{
	if (Message.SlotTag != SlotTag) return;
	Refresh();
}

void UGYEquipmentSlotWidget::Refresh()
{
	UEquipmentLoadoutComponent* Loadout = GetLoadout();

	FGuid InstanceId;
	const bool bHasItem = IsValid(Loadout) && Loadout->GetSlot(SlotTag, InstanceId) && InstanceId.IsValid();
	if (!bHasItem)
	{
		ClearView();
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	AGYPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<AGYPlayerState>() : nullptr;
	UInventoryComponent* Inventory = IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
	const FInventoryEntry* Entry = IsValid(Inventory) ? Inventory->FindEntry(InstanceId) : nullptr;
	if (Entry == nullptr)
	{
		ClearView();
		return;
	}

	FGYItemViewData View;
	View.Definition = Entry->Definition;
	View.InstanceId = Entry->InstanceId;
	View.GradeTag = Entry->GradeTag;
	View.Level = Entry->Level;
	View.Count = Entry->StackCount;
	View.StatDeviation = Entry->StatDeviation;
	View.RolledOptions = Entry->RolledOptions;
	SetView(View);
}

void UGYEquipmentSlotWidget::OnViewChanged(bool bIsEmpty)
{
	// 빈 슬롯이면 placeholder 실루엣 표시 (없으면 베이스가 숨김 처리)
	if (bIsEmpty && Image_Icon && !EmptySlotIcon.IsNull())
	{
		Image_Icon->SetOpacity(1.f);
		Image_Icon->SetBrushFromSoftTexture(EmptySlotIcon.LoadSynchronous(), false);
	}
}

FReply UGYEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭 → 패널에 위임 (캐릭터 창=해제 / 인첸트=대상 지정). 장착된 슬롯일 때만
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !CurrentInfo.Definition.IsNull())
	{
		if (UGYEquipmentPanelWidget* Panel = GetTypedOuter<UGYEquipmentPanelWidget>())
		{
			Panel->NotifySlotClicked(SlotTag, CurrentInfo.InstanceId);
			return FReply::Handled();
		}
	}

	// 우클릭 정보는 베이스가 처리
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
