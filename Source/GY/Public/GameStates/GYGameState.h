#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"
#include "GYGameState.generated.h"

UCLASS()
class GY_API AGYGameState : public AGameState
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


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

	void AddActiveQuest(FGameplayTag QuestTag);
	void RemoveActiveQuest(FGameplayTag QuestTag);

	FORCEINLINE const TArray<FGameplayTag>& GetCompletedQuests() const { return ClearedQuests; }
protected:
	virtual void BeginPlay() override;

private:
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
};
