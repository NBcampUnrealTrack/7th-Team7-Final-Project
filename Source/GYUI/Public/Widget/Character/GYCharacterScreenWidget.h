#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "GYCharacterScreenWidget.generated.h"

class UGYInventoryScreenWidget;
class UGYEquipmentPanelWidget;

// 통합 캐릭터 창 — 인벤토리(임베드) + 장비 페이퍼돌. 인벤 아이템 좌클릭 = 장착.
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYCharacterScreenWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 인벤 아이템 좌클릭 → 장착 요청 (장착 불가 아이템은 서버에서 무시)
	UFUNCTION()
	void HandleInventoryItemClicked(FGuid InstanceId);

	// 장비 슬롯 좌클릭 → 해제 요청
	UFUNCTION()
	void HandleEquipSlotClicked(FGameplayTag SlotTag, FGuid InstanceId);

	UPROPERTY()
	TObjectPtr<UGYInventoryScreenWidget> InventoryScreen;

	UPROPERTY()
	TObjectPtr<UGYEquipmentPanelWidget> EquipmentPanel;
};
