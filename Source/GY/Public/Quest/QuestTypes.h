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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FText QuestName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FText Description;

	// 활성화 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FGameplayTag ActivationEventTag;

	// 활성화 태그 대상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	FName ActivationTargetId = NAME_None;

	// 선행 퀘스트 ID 목록 (DataTable Row Name 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	TArray<FName> PrerequisiteIds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest")
	TArray<FQuestObjective> Objectives;

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
	FName QuestId = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	EQuestState State = EQuestState::NotStarted;

	// 목표별 현재 진행도 (인덱스 = FQuestObjective 배열 인덱스)
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> ObjectiveProgress;

	bool IsValid() const { return QuestId != NAME_None; }
};
