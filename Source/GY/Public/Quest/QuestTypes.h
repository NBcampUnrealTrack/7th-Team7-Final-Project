#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "QuestTypes.generated.h"

UENUM(BlueprintType)
enum class EQuestState : uint8
{
	NotStarted,
	InProgress,
	Completed,
	Failed
};

// 퀘스트 이벤트 데이터
// QuestManager가 받는 공통 이벤트 구조
USTRUCT(BlueprintType)
struct FQuestEventData
{
	GENERATED_BODY()

	// 이벤트 종류
	// 예:
	// Quest.Trigger.AreaEnter
	// Quest.Objective.Kill
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag EventTag;

	// 이벤트 대상
	// 예:
	// Zone1
	// Goblin
	// Blacksmith
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TargetId = NAME_None;

	// 진행 수량
	// 예:
	// 아이템 여러 개 획득
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 1;
};

// 퀘스트 목표 하나
USTRUCT(BlueprintType)
struct FQuestObjective
{
	GENERATED_BODY()

	// 목표 이벤트 종류
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag ObjectiveEventTag;

	// 대상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName TargetId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 RequiredCount = 1;

	bool IsValid() const
	{
		return ObjectiveEventTag.IsValid();
	}
};

// 퀘스트 보상
USTRUCT(BlueprintType)
struct FQuestReward
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Experience = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<class UObject>> ItemClasses; // TODO: 아이템 베이스 나오면 수정
};

// DataTable Row: 에디터에서 퀘스트 데이터를 관리하는 단위
USTRUCT(BlueprintType)
struct FQuestTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// 퀘스트 ID 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FGameplayTag QuestTag;

	// 퀘스트 제목
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FText QuestName;

	// 퀘스트 내용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest", meta=(MultiLine=true))
	FText Description;

	// 활성화 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FGameplayTag ActivationEventTag;

	// 활성화 태그 대상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FName ActivationTargetId = NAME_None;

	// 선행 퀘스트 ID 목록 (DataTable Row Name 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FGameplayTag PrerequisiteTag;

	// 퀘스트 목표
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FQuestObjective Objective;

	// 보상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FQuestReward Reward;
};

// 런타임 퀘스트 상태 (복제됨)
USTRUCT(BlueprintType)
struct FQuestRuntimeData
{
	GENERATED_BODY()

	// DataTable Row Name = 퀘스트 ID
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag QuestTag = {};

	UPROPERTY(BlueprintReadOnly)
	EQuestState State = EQuestState::NotStarted;

	// 목표 현재 진행도
	UPROPERTY(BlueprintReadOnly)
	int32 ObjectiveProgress = 0;

	bool IsValid() const { return QuestTag.IsValid(); }
};

// 네러티브 다이얼로그
USTRUCT(BlueprintType)
struct FDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag NarrativeTag;

	UPROPERTY(EditAnywhere)
	int32 Order = 0;

	UPROPERTY(EditAnywhere)
	bool bIsPlayer = false;

	UPROPERTY(EditAnywhere)
	FText Speaker;

	UPROPERTY(EditAnywhere)
	FText Dialogue;
};
