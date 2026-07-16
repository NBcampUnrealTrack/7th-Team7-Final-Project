#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Quest/QuestTypes.h"
#include "QuestSubsystem.generated.h"

struct FQuestEventMessage;
struct FGYQuestProgressMessage;

class AGYGameState;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuestStarted, FGameplayTag /*QuestTag*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuestProgressUpdated, FGameplayTag /*QuestTag*/, int32 /*NewCount*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuestCompleted, FGameplayTag /*QuestTag*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNarrativeDialogueStarted, TArray<FDialogueRow> /*Rows*/);

UCLASS()
class GY_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 퀘스트 정의 조회 (없으면 nullptr)
	const FQuestTableRow* FindQuestRow(FGameplayTag QuestTag) const;

	// 선행 퀘스트 조건 충족 여부 확인
	UFUNCTION(BlueprintCallable, Category="Quest")
	bool ArePrerequisitesMet(FGameplayTag QuestTag) const;

	// 퀘스트 이벤트 처리 (활성화 + 목표 진행 통합)
	UFUNCTION(BlueprintCallable, Category="Quest")
	void HandleQuestEvent(const FQuestEventData& EventData);

	// 런타임 퀘스트 상태 조회 (없으면 nullptr)
	const FQuestRuntimeData* GetQuestRuntimeData(FGameplayTag QuestTag) const;

	// 진행 중인 전체 퀘스트 조회
	const TMap<FGameplayTag, FQuestRuntimeData>& GetActiveQuests() const { return ActiveQuests; }

	// 월드 저장 복원용 — 브로드캐스트 없이 런타임 상태만 재구성 (서버 부팅, 플레이어 입장 전)
	void RestoreActiveQuests(const TMap<FGameplayTag, FQuestRuntimeData>& InActiveQuests) { ActiveQuests = InActiveQuests; }

	// DataTable에 등록된 전체 퀘스트 조회
	const TMap<FGameplayTag, const FQuestTableRow*>& GetAllQuestRows() const { return QuestCache; }

	AGYGameState* GetGYGameState() const;

	// NarrativeTag와 일치하는 다이얼로그 행을 Order 순으로 정렬해 UI에 브로드캐스트
	void BroadcastNarrativeDialogue(FGameplayTag NarrativeTag);

	// UI 바인딩용 델리게이트
	FOnQuestStarted OnQuestStarted;
	FOnQuestProgressUpdated OnQuestProgressUpdated;
	FOnQuestCompleted OnQuestCompleted;
	FOnNarrativeDialogueStarted OnNarrativeDialogueStarted;

private:
	// QuestTag → Row 포인터 캐시 (DataTable 수명에 종속)
	TMap<FGameplayTag, const FQuestTableRow*> QuestCache;

	// 진행 중인 퀘스트 런타임 상태
	TMap<FGameplayTag, FQuestRuntimeData> ActiveQuests;

	UPROPERTY()
	TObjectPtr<UDataTable> CachedQuestTable;

	UPROPERTY()
	TObjectPtr<UDataTable> CachedDialogueTable;

	void BuildCache(const UDataTable* DataTable);
	void OnQuestEvent(FGameplayTag Channel, const FQuestEventMessage& Message);
	void OnQuestStartedFromServer(FGameplayTag Channel, const FGYQuestProgressMessage& Message);
	void OnQuestCompletedFromServer(FGameplayTag Channel, const FGYQuestProgressMessage& Message);

	// ActiveQuests 갱신 + UI 델리게이트 브로드캐스트 (StartQuest/CompleteQuest, OnQuest***FromServer 공용)
	void MarkQuestStarted(FGameplayTag QuestTag);
	void MarkQuestCompleted(FGameplayTag QuestTag);

	FGameplayMessageListenerHandle QuestStartedListenerHandle;
	FGameplayMessageListenerHandle QuestCompletedListenerHandle;
	FGameplayMessageListenerHandle QuestEventListenerHandle;

	void GrantRewards(FGameplayTag QuestTag);

public:
	bool StartQuest(FGameplayTag QuestTag);
	void ProcessObjectiveProgress(const FQuestEventData& EventData);
	void CompleteQuest(FGameplayTag QuestTag);
};
