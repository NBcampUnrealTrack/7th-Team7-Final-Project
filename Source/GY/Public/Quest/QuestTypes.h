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

// 퀘스트 목표 하나
USTRUCT(BlueprintType)
struct FQuestObjective
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag EventTag;

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
	TArray<TSubclassOf<class UObject>> ItemClasses;
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