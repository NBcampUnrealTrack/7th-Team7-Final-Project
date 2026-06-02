#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "GYLootBoxScreenWidget.generated.h"

class UGYLootDropSlotWidget;
class UPanelWidget;
class ALootBoxActor;
struct FGYLootBoxStateMessage;

UCLASS(Abstract, Blueprintable)
class GYUI_API UGYLootBoxScreenWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	// 박스 점유 직후 BP가 호출 (Message.Loot.ShowBox 구독 → push → BindToBox)
	UFUNCTION(BlueprintCallable, Category = "GY|Loot")
	void BindToBox(ALootBoxActor* Box);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Loot")
	TSubclassOf<UGYLootDropSlotWidget> DropSlotWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotContainer;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Loot")
	void OnBoxRefreshed(int32 RemainingDrops);

private:
	void Refresh();
	void ReleaseOccupancy();
	void HandleBoxStateChanged(FGameplayTag Channel, const FGYLootBoxStateMessage& Msg);

	UFUNCTION()
	void HandleBoxDestroyed(AActor* DestroyedActor);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGYLootDropSlotWidget>> SlotWidgets;

	TWeakObjectPtr<ALootBoxActor> BoundBox;
	FGameplayMessageListenerHandle ListenerHandle;
};
