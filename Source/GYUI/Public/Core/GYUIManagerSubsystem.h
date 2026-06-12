#pragma once

#include "AbilitySystemComponent.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "GYUIManagerSubsystem.generated.h"

class AGYPlayerController;
class UCommonActivatableWidget;
class UGYPrimaryGameLayout;
struct FGYRegionEnteredMessage;
struct FGYRegionExitedMessage;
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
		TSubclassOf<UCommonActivatableWidget> WidgetClass);


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

	/** 현재 활성화된 PrimaryGameLayout 참조 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GY|UI")
	UGYPrimaryGameLayout* GetPrimaryGameLayout() const { return PrimaryGameLayout; }

	/** 어디서든 호출 가능한 정적 접근자 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI", meta = (WorldContext = "WorldContextObject"))
	static UGYUIManagerSubsystem* Get(const UObject* WorldContextObject);

	/** 캐시된 이름 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GY|UI")
	FString GetPlayerName(APlayerState* PS) const;

	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	TArray<APlayerState*> GetKnownPlayerStates() const;

protected:
	void UnbindASC();

	UPROPERTY(Transient)
	TObjectPtr<UGYPrimaryGameLayout> PrimaryGameLayout;

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
	};

	TMap<FGameplayTag, FTagWidgetEntry> TagWidgetMap;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	/** 클래스 단위 토글 위젯 활성 상태 추적 */
	TMap<TSubclassOf<UCommonActivatableWidget>, TWeakObjectPtr<UCommonActivatableWidget>> ToggleWidgetMap;

	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);

	void HandlePlayerStateInitialized(AGYPlayerController* PC);

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

	/** 부활 브로드캐스트 */
	void StartRevivalBroadcast();
	void StopRevivalBroadcast();
	void TickRevivalBroadcast();
	void BroadcastRevivalProgress(float Current, float Max) const;

	FDelegateHandle DeathTagHandle;
	FTimerHandle RevivalTickHandle;
	float RevivalElapsed = 0.f;
	float RevivalDuration = 0.f;
	bool bRevivalActive = false;

	UPROPERTY(EditDefaultsOnly, Category = "GY|UI")
	float RevivalBroadcastInterval = 0.05f;
};
