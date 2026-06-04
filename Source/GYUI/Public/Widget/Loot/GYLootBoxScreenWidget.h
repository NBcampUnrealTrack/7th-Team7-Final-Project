#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "GYLootBoxScreenWidget.generated.h"

class UGYLootDropSlotWidget;
class UPanelWidget;
class UButton;
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

	// 고정 그리드 칸 수. 드롭이 적으면 나머지는 빈 칸, 더 많으면 이 값 이상으로 늘려 숨기지 않음
	UPROPERTY(EditDefaultsOnly, Category = "GY|Loot", meta = (ClampMin = 1))
	int32 GridSlotCount = 8;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotContainer;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UButton> Btn_Close;

	UFUNCTION(BlueprintImplementableEvent, Category = "GY|Loot")
	void OnBoxRefreshed(int32 RemainingDrops);

private:
	void EnsureSlots();
	void Refresh();
	void ReleaseOccupancy();
	void HandleBoxStateChanged(FGameplayTag Channel, const FGYLootBoxStateMessage& Msg);

	UFUNCTION()
	void HandleBoxDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGYLootDropSlotWidget>> SlotWidgets;

	TWeakObjectPtr<ALootBoxActor> BoundBox;
	FGameplayMessageListenerHandle ListenerHandle;
};
