#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "Account/GYAccountSubsystem.h"
#include "GYSessionMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UGYSessionCardWidget;
class UPanelWidget;
class UTextBlock;
class UWidgetSwitcher;

// 세션 패널 — 목록(내 세션/모든 세션) + 생성 UI 전환. 메인메뉴의 "세션" 버튼이 띄우는 본체.
// WBP_SessionContainer 가 이 클래스를 부모로 reparent 하면 아래 이름의 위젯을 채택한다:
//   [공통] ModeSwitcher(WidgetSwitcher, 0=목록 1=생성) / StatusText(TextBlock)
//   [목록] MySessionList(PanelWidget) / AllSessionList(PanelWidget)
//          CreateOpenButton(Button) / RefreshButton(Button)
//   [생성] NameInputBox(EditableTextBox) / CreateConfirmButton(Button) / CreateCancelButton(Button)
// 카드 스타일은 SessionCardClass 에 WBP_SessionWidget(reparent 후) 지정.
// 전부 없어도 동작 — 코드 구성 폴백 내장 (dev: gy.UI.SessionMenu 콘솔로 단독 확인)
UCLASS()
class GYUI_API UGYSessionMenuWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYSessionMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, Category = "Session")
	TSubclassOf<UGYSessionCardWidget> SessionCardClass;

private:
	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleCreateOpenClicked();

	UFUNCTION()
	void HandleCreateConfirmClicked();

	UFUNCTION()
	void HandleCreateCancelClicked();

	void ResolveOrBuildWidgets();
	void BuildFallbackPanel();
	void SetMode(int32 SwitcherIndex);

	void RefreshSessions();
	void OnWorldList(bool bSuccess, const TArray<FGYWorldSummary>& Worlds);
	void OnJoinPhase(int64 WorldId, EGYJoinWorldPhase Phase);
	void OnAccountReady(bool bSuccess);
	void JoinWorld(int64 WorldId);
	void SetStatus(const FString& Message);
	void SetCardsEnabled(bool bEnabled);
	UGYSessionCardWidget* AddCard(UPanelWidget* Container, const FGYWorldSummary& World);

	UGYAccountSubsystem* ResolveAccount() const;

	// BindWidgetOptional: WBP 가 같은 이름의 위젯 변수를 두면 여기에 바인딩 (BP 프로퍼티 중복 생성 충돌 방지)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> ModeSwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> MySessionList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> AllSessionList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> NameInputBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TArray<TObjectPtr<UGYSessionCardWidget>> Cards;

	FDelegateHandle AccountReadyHandle;
	FDelegateHandle JoinPhaseHandle;
	bool bJoinInProgress = false;
};
