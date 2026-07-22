#pragma once

#include "CoreMinimal.h"
#include "WorldSession/GYWorldSessionSubsystem.h"
#include "Widget/MainMenu/GYSlidePanelWidget.h"
#include "GYSessionContainerWidget.generated.h"

class UButton;
class UGYAccountSubsystem;
class UGYJoinStatusWidget;
class UGYSessionCardWidget;
class UGYSessionCreateWidget;
class UScrollBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGYCreateSessionRequested);

// 세션(월드) 목록 패널 — 조회/입장 + 생성 패널 열기. 디자인/슬라이드는 WBP_SessionContainer 소유.
// WBP 위젯 계약: Button_CreateSession / Button_Back (BindWidget, 팀원 원안)
//               Button_Refresh (BindWidgetOptional — 목록 수동 새로고침)
//               SessionScrollBox (ScrollBox — 코드가 참가중/모든 섹션과 카드를 채움)
UCLASS(Blueprintable)
class GYUI_API UGYSessionContainerWidget : public UGYSlidePanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GY|Session")
	FGYCreateSessionRequested OnCreateSessionRequested;

	// 목록 다시 조회 (패널이 열릴 때/생성 직후 호출)
	UFUNCTION(BlueprintCallable, Category = "GY|Session")
	void RefreshSessions();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_CreateSession;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	// 목록 수동 새로고침 — WBP에 없어도 되도록 Optional
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Refresh;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SessionScrollBox;

	// 카드 위젯 (WBP_SessionWidget — UGYSessionCardWidget 파생)
	UPROPERTY(EditAnywhere, Category = "GY|Session")
	TSubclassOf<UGYSessionCardWidget> SessionCardClass;

	// 생성 패널 — 콘솔 단독 실행 폴백용 (메뉴 플로우에선 OnCreateSessionRequested 구독자가 소유)
	UPROPERTY(EditAnywhere, Category = "GY|Session")
	TSubclassOf<UGYSessionCreateWidget> CreatePanelClass;

	// 입장 진행 모달 (대기열/준비/접속 + 취소)
	UPROPERTY(EditAnywhere, Category = "GY|Session")
	TSubclassOf<UGYJoinStatusWidget> JoinStatusClass;

private:
	UFUNCTION() void HandleCreateSessionClicked();
	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleRefreshClicked();

	void OnWorldList(bool bSuccess, const TArray<FGYWorldSummary>& Worlds);
	void OnJoinPhase(int64 WorldId, EGYJoinWorldPhase Phase);
	void OnAccountReady(bool bSuccess);
	void JoinWorld(int64 WorldId);
	void RequestDeleteWorld(int64 WorldId);
	void ConfirmDeleteWorld(int64 WorldId);
	void SetCardsEnabled(bool bEnabled);
	void AddSectionHeader(const FString& Label);
	void AddCard(const FGYWorldSummary& World, bool bMine);

	UGYAccountSubsystem* ResolveAccount() const;
	UGYWorldSessionSubsystem* ResolveSession() const;

	UPROPERTY()
	TArray<TObjectPtr<UGYSessionCardWidget>> Cards;

	UPROPERTY()
	TObjectPtr<UGYJoinStatusWidget> JoinStatusModal;

	UPROPERTY()
	TObjectPtr<UGYSessionCreateWidget> CreatePanel;

	FDelegateHandle AccountReadyHandle;
	FDelegateHandle JoinPhaseHandle;
	bool bJoinInProgress = false;
};
