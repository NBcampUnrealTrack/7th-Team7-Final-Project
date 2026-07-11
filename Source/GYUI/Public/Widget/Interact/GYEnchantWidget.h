// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/Interact/GYTimeRiftPanelWidget.h"
#include "GYEnchantWidget.generated.h"

class UProgressBar;
class UButton;
class UCommonTextBlock;
class UGYEnchantSlotWidget;
class UGYInventoryScreenWidget;
class UGYEquipmentPanelWidget;
class UGYItemInfoWidget;
/**
 *
 */
UCLASS()
class GYUI_API UGYEnchantWidget : public UGYTimeRiftPanelWidget
{
	GENERATED_BODY()
public:
	UFUNCTION()
	void OnCurrencyChanged(FGameplayTag GameplayTag, int32 Amount);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 인첸트 대상 지정 (클릭/드롭 공통 진입점) — 슬롯 표시 + 전용 옵션 패널 갱신
	void SetTarget(const FGuid& InstanceId);

protected:
	virtual FGameplayTag GetExitEventTag() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGYEnchantSlotWidget> EnchantSlotWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExecuteButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;

	// 대상 아이템 옵션 전용 표시 패널 (pinned). 없어도 동작
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UGYItemInfoWidget> TargetInfo;

	// 시간의 파편 보유/필요량 숫자 표시
	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_TimeShard;
private:
	UFUNCTION()
	void OnCloseButtonClicked();
	UFUNCTION()
	void OnExecuteButtonClicked();

	// 인벤 아이템 좌클릭 → 인첸트 대상 지정
	UFUNCTION()
	void HandleInventoryItemClicked(FGuid InstanceId);

	// 장착 아이템 좌클릭 → 인첸트 대상 지정 (장착 해제 없이 그대로 리롤)
	UFUNCTION()
	void HandleEquipSlotClicked(FGameplayTag SlotTag, FGuid InstanceId);

	FDelegateHandle OnCurrencyChangedHandle;

	UPROPERTY()
	TObjectPtr<UGYInventoryScreenWidget> InventoryScreen;

	UPROPERTY()
	TObjectPtr<UGYEquipmentPanelWidget> EquipmentPanel;
};
