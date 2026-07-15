#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"
#include "GYGameState.generated.h"

struct FGYWorldResetMessage;
class UGYExperienceManagerComponent;
class UWorldSaveComponent;

UCLASS()
class GY_API AGYGameState : public AGameState
{
	GENERATED_BODY()

public:
	AGYGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UGYExperienceManagerComponent* GetExperienceManagerComponent() const;
	UWorldSaveComponent* GetWorldSaveComponent() const;


	FORCEINLINE float GetCurrentTime() const { return CurrentTime; }
	FORCEINLINE void SetCurrentTime(float InCurrentTime);
	FORCEINLINE float GetTimeScale() const { return TimeScale; }
	FORCEINLINE void SetTimeScale(float InTimeScale) { TimeScale = InTimeScale; }
	FORCEINLINE float GetMidnight() const { return Midnight; }
	FORCEINLINE float GetWorldLevel() const { return WorldLevel; }
	FORCEINLINE void SetWorldLevel(float InWorldLevel) { WorldLevel = InWorldLevel; }

	UFUNCTION()
	void OnRep_CurrentTime();
	UFUNCTION()
	void OnRep_TimeScale();
	UFUNCTION()
	void OnRep_ClearedQuests();
	UFUNCTION()
	void OnRep_ActiveQuests();

	// 모든 목표 달성 여부 확인
	UFUNCTION(BlueprintCallable, Category="Quest")
	bool IsQuestComplete(const FGameplayTag QuestTag) const;

	// 퀘스트 클리어 추가
	UFUNCTION(BlueprintCallable, Category="Quest")
	void AddCompletedQuest(FGameplayTag QuestTag);

	UFUNCTION()
	void NotifyWorldResetAll(float DurationOverride);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayWorldResetSequence(float DurationOverride);

	void AddActiveQuest(FGameplayTag QuestTag);
	void RemoveActiveQuest(FGameplayTag QuestTag);

	FORCEINLINE const TArray<FGameplayTag>& GetCompletedQuests() const { return ClearedQuests; }
	FORCEINLINE const TArray<FGameplayTag>& GetActiveQuests() const { return ActiveQuestTags; }

	// 클라 로컬에서 즉시 재생(데디 서버는 내부에서 스킵).
	UFUNCTION()
	void PlayGameBGMLocal(FGameplayTag BGMTag) const;

	// 게임 시작 시점(GameMode)에서 한 번 호출 - bGameBGMStarted 복제/OnRep을 통해 서버/모든 클라(늦게 접속해도)에서 재생
	void StartGameBGM();

	UFUNCTION()
	void OnRep_GameBGMStarted();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UGYExperienceManagerComponent> ExperienceManagerComponent;

	UPROPERTY()
	TObjectPtr<UWorldSaveComponent> WorldSaveComponent;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentTime)
	float CurrentTime;
	//06:00~24:00 <- 20minute
	UPROPERTY(ReplicatedUsing=OnRep_TimeScale)
	float TimeScale;

	const float Midnight = 24.f * 60.f * 60.f;

	UPROPERTY()
	float WorldLevel = 1;

	/** 시간 변경 시 GMS 브로드캐스트 */
	void BroadcastTimeChanged();
	int32 LastBroadcastedMinute = -1;

	UPROPERTY(ReplicatedUsing=OnRep_ClearedQuests)
	TArray<FGameplayTag> ClearedQuests;

	int32 PreviousClearedCount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveQuests)
	TArray<FGameplayTag> ActiveQuestTags;

	TArray<FGameplayTag> PreviousActiveQuestTags;

	UPROPERTY(ReplicatedUsing=OnRep_GameBGMStarted)
	bool bGameBGMStarted = false;
};
