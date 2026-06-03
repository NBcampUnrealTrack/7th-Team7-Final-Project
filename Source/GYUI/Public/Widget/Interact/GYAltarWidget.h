// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/AltarStorageComponent.h"
#include "GYAltarWidget.generated.h"

struct FGYInventoryEntryMessage;
class UGYItemSlotWidget;
class UButton;
/**
 *
 */
UCLASS()
class GYUI_API UGYAltarWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Inventory")
	TSubclassOf<UGYItemSlotWidget> SlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Inventory", meta = (ClampMin = 1))
	int32 GridSlotCount = 48;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotContainer;


	UFUNCTION()
	void OnCloseButtonClicked();
private:
	void EnsureSlots();
	void Refresh();
	UAltarStorageComponent* ResolveAltarStorage() const;
	void HandleEntryChanged(FGameplayTag Channel, const FGYInventoryEntryMessage& Msg);


	UPROPERTY(Transient)
	TArray<TObjectPtr<UGYItemSlotWidget>> SlotWidgets;

	FGameplayMessageListenerHandle ListenerHandle;
};
