// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/Interact/GYTimeRiftPanelWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/AltarStorageComponent.h"
#include "GYAltarWidget.generated.h"

struct FGYInventoryEntryMessage;
class UGYItemSlotWidget;
class UGYInventoryScreenWidget;
class UButton;
class UProgressBar;
class UCommonTextBlock;
/**
 *
 */
UCLASS()
class GYUI_API UGYAltarWidget : public UGYTimeRiftPanelWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Inventory")
	TSubclassOf<UGYItemSlotWidget> SlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Inventory", meta = (ClampMin = 1))
	int32 GridSlotCount = 48;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExecuteButton;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_TimeShard;

	UFUNCTION()
	void OnCloseButtonClicked();
	UFUNCTION()
	void OnExecuteButtonClicked();

protected:
	virtual FGameplayTag GetExitEventTag() const override;

private:
	void EnsureSlots();
	void Refresh();
	UAltarStorageComponent* ResolveAltarStorage() const;
	void HandleEntryChanged(FGameplayTag Channel, const FGYInventoryEntryMessage& Msg);

	void UpdateTimeShardBar(int32 Amount);
	void HandleAltarSlotClicked(const FGuid& InstanceId);
	void TransferItem(FGameplayTag FromTag, const FGuid& InstanceId, FGameplayTag ToTag);

	UFUNCTION()
	void OnCurrencyChanged(FGameplayTag CurrencyTag, int32 Amount);

	UFUNCTION()
	void HandleInventoryItemClicked(FGuid InstanceId);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGYItemSlotWidget>> SlotWidgets;

	UPROPERTY()
	TScriptInterface<IItemContainer> Container;

	UPROPERTY()
	TObjectPtr<UGYInventoryScreenWidget> InventoryScreen;

	FGameplayMessageListenerHandle ListenerHandle;
	FDelegateHandle OnCurrencyChangedHandle;

	int32 GetTimeShardMax() const;
};
