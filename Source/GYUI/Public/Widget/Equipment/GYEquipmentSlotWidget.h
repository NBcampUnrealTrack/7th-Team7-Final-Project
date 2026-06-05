#pragma once

#include "CoreMinimal.h"
#include "Widget/Slot/GYItemSlotBase.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "GYEquipmentSlotWidget.generated.h"

class UTexture2D;
class UEquipmentLoadoutComponent;
struct FGYEquipSlotMessage;

// 캐릭터 창 페이퍼돌 장비 슬롯. 해당 SlotTag의 장착 상태를 표시하고, 좌클릭으로 해제·우클릭으로 정보 표시.
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYEquipmentSlotWidget : public UGYItemSlotBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void OnViewChanged(bool bIsEmpty) override;

	// 이 위젯이 대표하는 장비 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|Equip", meta = (Categories = "Equipment.Slot"))
	FGameplayTag SlotTag;

	// 빈 슬롯 placeholder 아이콘 (슬롯별 실루엣)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|Equip")
	TSoftObjectPtr<UTexture2D> EmptySlotIcon;

private:
	void Refresh();
	void HandleLoadoutChanged(FGameplayTag Channel, const FGYEquipSlotMessage& Message);
	UEquipmentLoadoutComponent* GetLoadout() const;

	FGameplayMessageListenerHandle ListenerHandle;
};
