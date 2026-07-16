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
	UGYLootBoxScreenWidget(const FObjectInitializer& ObjectInitializer);

	// 박스 점유 직후 BP가 호출 (Message.Loot.ShowBox 구독 → push → BindToBox)
	UFUNCTION(BlueprintCallable, Category = "GY|Loot")
	void BindToBox(ALootBoxActor* Box);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 활성화 시 위젯에 키보드 포커스를 직접 부여
	virtual void NativeOnActivated() override;

	// 상호작용 키로 다시 닫기
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// F/ESC 키 이벤트를 받으려면 위젯 자신이 포커스를 가져야 함
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

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

	// 활성화 직후 위젯에 키보드 포커스를 실제로 확보할 때까지 몇 프레임 재시도
	void ScheduleFocusAttempt();
	void EnsureKeyboardFocus();

	UFUNCTION()
	void HandleBoxDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGYLootDropSlotWidget>> SlotWidgets;

	TWeakObjectPtr<ALootBoxActor> BoundBox;
	FGameplayMessageListenerHandle ListenerHandle;

	// 포커스 확보 재시도 프레임 수와 현재 시도 횟수
	static constexpr int32 MaxFocusRetries = 10;
	int32 FocusRetryCount = 0;
};
