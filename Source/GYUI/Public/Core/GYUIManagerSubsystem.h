#pragma once

#include "AbilitySystemComponent.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "GYUIManagerSubsystem.generated.h"

struct FGYCinematicMessage;
class AGYPlayerController;
class UCommonActivatableWidget;
class UGYPrimaryGameLayout;
class UGYEndingCreditsWidget;
class UGYInteractionWaitingWidget;
class UGYWorldResetWidget;
class UGYEndingNarrativeWidget;
struct FGYRegionEnteredMessage;
struct FGYRegionExitedMessage;
struct FGYEndingCinematicFinishedMessage;
struct FGYEndingCreditsFinishedMessage;
struct FGYInteractionWaitingMessage;
struct FGYEndingStartedMessage;
struct FGYIntroCinematicMessage;
struct FGYWorldResetMessage;
struct FGYClockOverlayMessage;
struct FGYReviveHoldMessage;
struct FGYEndingNarrativeFinishedMessage;
struct FGYBossStateMessage;
/**
 * 로컬마다 생성, 관리되는 UI 총괄 매니저
 */
UCLASS()
class GYUI_API UGYUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	//PlayerState의 태그 구독
	void BindASC(UAbilitySystemComponent* InASC);

	void RegisterTagDrivenWidget(
		FGameplayTag StateTag,
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass,
		bool bBlocksOtherWidgets = true);


	/** PrimaryGameLayout 생성 후 화면 띄움 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void CreatePrimaryGameLayout(TSubclassOf<UGYPrimaryGameLayout> LayoutClass);

	/** 뷰포트에 추가된 레이아웃 제거, 참조 해제 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void RemovePrimaryGameLayout();

	/** 지정된 Layer 컨테이너에 위젯 추가 및 Activate */
	UFUNCTION(BlueprintCallable, Category = "GY|UI", meta = (DeterminesOutputType = "WidgetClass"))
	UCommonActivatableWidget* PushWidgetToLayer(FGameplayTag LayerTag,
	                                            TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 화면에 띄워진 위젯 레이어에서 제거 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void PopWidget(UCommonActivatableWidget* Widget);

	/** 클래스 단위 토글 — 이미 떠 있으면 pop, 아니면 push */
	UFUNCTION(BlueprintCallable, Category = "GY|UI", meta = (DeterminesOutputType = "WidgetClass"))
	UCommonActivatableWidget* ToggleWidgetInLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 현재 Menu 레이어에 열려 있는 일반 메뉴 위젯을 전부 닫음 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void CloseAllMenus();

	/** ESC/뒤로가기 단일 처리 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void HandleUIBack();

	/** UI가 잠겨 있는지 여부 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GY|UI")
	bool IsSystemUIActive() const;

	/** 시네마틱/연출 등 게임플레이 쪽에서 위젯 열기를 잠글 때 사용 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void PushUIInteractionBlock(FGameplayTag Reason);

	/** PushUIInteractionBlock 해제 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void PopUIInteractionBlock(FGameplayTag Reason);

	/** 현재 활성화된 PrimaryGameLayout 참조 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GY|UI")
	UGYPrimaryGameLayout* GetPrimaryGameLayout() const { return PrimaryGameLayout; }

	/** 어디서든 호출 가능한 정적 접근자 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI", meta = (WorldContext = "WorldContextObject"))
	static UGYUIManagerSubsystem* Get(const UObject* WorldContextObject);

	/** 로컬 플레이어가 보스전 중인지 여부 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GY|UI")
	bool IsBossFightActive() const { return bBossFightActive; }

	/** 캐시된 이름 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GY|UI")
	FString GetPlayerName(APlayerState* PS) const;

	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	TArray<APlayerState*> GetKnownPlayerStates() const;

	UFUNCTION(BlueprintCallable, Category = "GY|UI|Ending")
	void StartEndingCredits();

	UFUNCTION(BlueprintCallable, Category = "GY|UI|Ending")
	void StartEndingNarrative();

	UPROPERTY(EditDefaultsOnly, Category = "GY|UI|Ending")
	float EndingCrossfadeHold = 0.75f;

protected:
	void UnbindASC();

	UPROPERTY(Transient)
	TObjectPtr<UGYPrimaryGameLayout> PrimaryGameLayout;

	TWeakObjectPtr<UCommonActivatableWidget> HUDWidget;

	/** 플레이어 명단 동기화 주기 */
	UPROPERTY(EditDefaultsOnly, Category = "GY|UI", meta = (ClampMin = "0.1"))
	float RosterSyncInterval = 0.5f;

private:
	struct FTagWidgetEntry
	{
		FGameplayTag LayerTag;
		TSubclassOf<UCommonActivatableWidget> WidgetClass;
		TWeakObjectPtr<UCommonActivatableWidget> ActiveWidget;
		FDelegateHandle DelegateHandle;
		/** 이 위젯이 떠 있는 동안 일반 메뉴 열기를 막을지 */
		bool bBlocksOtherWidgets = true;
	};

	TMap<FGameplayTag, FTagWidgetEntry> TagWidgetMap;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	/** 클래스 단위 토글 위젯 활성 상태 추적 */
	TMap<TSubclassOf<UCommonActivatableWidget>, TWeakObjectPtr<UCommonActivatableWidget>> ToggleWidgetMap;

	/** 현재 Menu 레이어에 열려 있는 일반 메뉴 위젯들 */
	TArray<TWeakObjectPtr<UCommonActivatableWidget>> ActiveMenuWidgets;

	/** 현재 떠 있는 시스템 위젯들 */
	TArray<TWeakObjectPtr<UCommonActivatableWidget>> ActiveSystemWidgets;

	/** 위젯이 아닌 게임플레이 잠금 사유 */
	TSet<FGameplayTag> ActiveUIBlockReasons;

	/** HandleUIBack 중복 처리 방지용 */
	uint64 LastUIBackFrame = 0;

	/** 정책 없이 레이어에 그대로 push */
	UCommonActivatableWidget* PushWidgetRaw(FGameplayTag LayerTag,
	                                        TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 시스템 위젯 push */
	UCommonActivatableWidget* PushSystemWidget(FGameplayTag LayerTag,
	                                           TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 일반 메뉴 위젯을 추적 목록에 등록 */
	void TrackMenuWidget(UCommonActivatableWidget* Widget);

	bool HasBlockingSystemWidget() const;

	/** 이동, 공격을 막아야 하는 UI 상태인지 */
	bool ShouldBlockGameplayInput() const;

	/** 현재 UI 상태에 맞춰 이동, 공격 차단 태그를 로컬 ASC에 반영 */
	void RefreshGameplayInputBlock();
	void SetGateWaitingBlock(bool bBlock);

	/** State.UI.MenuOpen 태그를 실제로 붙였는지 추적 */
	bool bGameplayInputBlockApplied = false;
	bool bGateWaitingBlockApplied = false;

	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void HandlePlayerStateInitialized(AGYPlayerController* PC);
	UFUNCTION()
	void HandleLocalControllerReady(AGYPlayerController* PC);
	void EnsurePrimaryLayoutAndHUD();

	/** 스탯 브로드캐스트 정보 묶어서 관리 */
	struct FStatBroadcastEntry
	{
		FGameplayTag Channel;
		FGameplayAttribute CurrentAttribute;
		FGameplayAttribute MaxAttribute;
		FDelegateHandle CurrentHandle;
		FDelegateHandle MaxHandle;
	};
	TArray<FStatBroadcastEntry> StatBroadcastEntries;

	/** 스탯 정보 송신 */
	void RegisterStatBroadcast(UAbilitySystemComponent* ASC);
	void UnregisterStatBroadcast();
	void OnStatAttributeChanged(const FOnAttributeChangeData& Data);
	void BroadcastStat(UAbilitySystemComponent* ASC, const FStatBroadcastEntry& Entry);
	/** 경험치, 레벨 정보 송신 */
	void BroadcastXP();
	void OnXPRelatedChanged(const FOnAttributeChangeData& Data);
	FDelegateHandle LevelHandle;
	FDelegateHandle XPHandle;

	/** 알려진 PlayerState별 마지막 이름 */
	TMap<TWeakObjectPtr<APlayerState>, FString> KnownPlayerNames;
	FTimerHandle RosterSyncHandle;

	/** 플레이어 명단 동기화 */
	void SyncPlayerRoster();
	APlayerState* GetLocalPlayerState() const;

    /** Region 진입/나가기, 보스 데이터 */
	void HandleRegionEntered(FGameplayTag, const FGYRegionEnteredMessage& Msg);
	void HandleRegionExited(FGameplayTag, const FGYRegionExitedMessage& Msg);

	FGameplayMessageListenerHandle RegionEnterListenerHandle;
	FGameplayMessageListenerHandle RegionExitListenerHandle;

	FGameplayTag ActiveRegionId;

	/** 시계 오버레이 - 휴식/부활 공용 연출 */
	void HandleClockOverlay(FGameplayTag, const FGYClockOverlayMessage& Msg);
	void PlayClockOverlay(float HoldDuration, FGameplayTag Reason);
	void StopClockOverlay();
	void RequestStopClockOverlay();
	void HandleClockOverlayFinished();

	FDelegateHandle DeathTagHandle;
	FGameplayMessageListenerHandle ClockOverlayHandle;
	TWeakObjectPtr<UGYWorldResetWidget> ActiveClockOverlayWidget;

	/** 보스전 상태 추적 */
	void HandleBossState(FGameplayTag, const FGYBossStateMessage& Msg);
	FGameplayMessageListenerHandle BossStateListenerHandle;
	bool bBossFightActive = false;

	/** 엔딩 흐름 */
	void HandleEndingStarted(FGameplayTag, const FGYEndingStartedMessage& Msg);
	void HandleEndingWaiting(FGameplayTag, const FGYInteractionWaitingMessage& Msg);
	void HandleEndingCinematicFinished(FGameplayTag, const FGYEndingCinematicFinishedMessage& Msg);
	void HandleEndingNarrativeFinished(FGameplayTag, const FGYEndingNarrativeFinishedMessage&);
	void HandleEndingCreditsFinished(FGameplayTag, const FGYEndingCreditsFinishedMessage& Msg);
	void TravelToMainMenu() const;

	FGameplayMessageListenerHandle EndingStartedHandle;
	FGameplayMessageListenerHandle EndingWaitingHandle;
	FGameplayMessageListenerHandle EndingCinematicFinishedHandle;
	FGameplayMessageListenerHandle EndingNarrativeFinishedHandle;
	FGameplayMessageListenerHandle EndingCreditsFinishedHandle;

	TWeakObjectPtr<UCommonActivatableWidget> ActiveCreditsWidget;
	TWeakObjectPtr<UCommonActivatableWidget> ActiveNarrativeWidget;
	TWeakObjectPtr<UGYInteractionWaitingWidget> ActiveWaitingWidget;

	void HandleToggleSettings(FGameplayTag Tag, const struct FGYToggleSettingsMessage& Msg);
	FGameplayMessageListenerHandle ToggleSettingsListenerHandle;

	void HandleEnterCinematic(FGameplayTag, const FGYCinematicMessage& Msg);
	FGameplayMessageListenerHandle EnterCinematicHandle;

	/** 부활 진행 위젯 */
	void HandleRevivalTagChanged(const FGameplayTag Tag, int32 NewCount);
	void UpdateRevivalWidget();

	FDelegateHandle RevivingTagHandle;
	FDelegateHandle BeingRevivedTagHandle;
	TWeakObjectPtr<UCommonActivatableWidget> ActiveRevivalWidget;

	/** 다운 상태 포기 키 홀드 */
	void HandleReviveHold(FGameplayTag, const FGYReviveHoldMessage& Msg);
	void TickGiveUpProgress();
	void StopGiveUpProgress();

	FGameplayMessageListenerHandle ReviveHoldHandle;
	bool bReviveHoldActive = false;
	float GiveUpHoldStartTime = 0.f;
	float GiveUpHoldDuration = 0.f;
	FTimerHandle GiveUpProgressTimerHandle;
	FTimerHandle NarrativeCleanupTimerHandle;
};
