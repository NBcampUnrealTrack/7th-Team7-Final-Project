#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYEquipmentPanelWidget.generated.h"

// 장비 슬롯 좌클릭 시 발행. 호스트(캐릭터 창=해제 / 인첸트=대상 지정)가 바인딩해 동작 결정
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGYOnEquipSlotClicked, FGameplayTag, SlotTag, FGuid, InstanceId);

// 6칸 페이퍼돌을 담는 재사용 장비 패널. 슬롯은 직접 동작하지 않고 클릭을 이 패널로 위임한다.
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYEquipmentPanelWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 자식 장비 슬롯이 좌클릭 시 호출 → OnSlotClicked 발행
	void NotifySlotClicked(FGameplayTag SlotTag, const FGuid& InstanceId);

	UPROPERTY(BlueprintAssignable, Category = "GY|Equip")
	FGYOnEquipSlotClicked OnSlotClicked;
};
