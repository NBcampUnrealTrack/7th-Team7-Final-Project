#include "Quest/QuestSubsystem.h"
#include "Quest/QuestSettings.h"
#include "Engine/DataTable.h"
#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UQuestSettings* Settings = UQuestSettings::Get();
	if (!Settings)
	{
		return;
	}

	UDataTable* DataTable = Settings->QuestDataTable.LoadSynchronous();
	if (!DataTable)
	{
		GY_WARN(Content, CYS, "UQuestSubsystem: QuestDataTable이 설정되지 않았습니다.");
		return;
	}

	BuildCache(DataTable);
}

void UQuestSubsystem::BuildCache(const UDataTable* DataTable)
{
	// 데이터 테이블 순회 및 캐싱
	QuestCache.Empty();

	for (const FName& RowName : DataTable->GetRowNames())
	{
		const FQuestTableRow* Row = DataTable->FindRow<FQuestTableRow>(RowName, TEXT("UQuestSubsystem::BuildCache"));
		if (Row)
		{
			if (QuestCache.Contains(Row->QuestTag))
			{
				GY_WARN(Content, CYS, "중복 퀘스트 태그 발견 : %s", *Row->QuestTag.ToString());
				continue;
			}
			QuestCache.Add(Row->QuestTag, Row);
		}
	}

	GY_LOG(Content, CYS, "UQuestSubsystem: 퀘스트 %d개 로드됨", QuestCache.Num());
}

const FQuestTableRow* UQuestSubsystem::FindQuestRow(FGameplayTag QuestTag) const
{
	const FQuestTableRow* const* Found = QuestCache.Find(QuestTag);
	return Found ? *Found : nullptr;
}

bool UQuestSubsystem::ArePrerequisitesMet(FGameplayTag QuestTag) const
{
	const FQuestTableRow* Row = FindQuestRow(QuestTag);
	if (!Row)
	{
		return false;
	}
	if (!Row->PrerequisiteTag.IsValid())
	{
		return true;
	}
	AGYGameState* GameState = GetGYGameState();
	if (!GameState)
	{
		return false;
	}
	return GameState->IsQuestComplete(Row->PrerequisiteTag);
}

void UQuestSubsystem::HandleQuestEvent(const FQuestEventData& EventData)
{
	// 활성화 조건 체크 → 새 퀘스트 시작
	for (const auto& Pair : QuestCache)
	{
		const FQuestTableRow* Row = Pair.Value;
		if (Row->ActivationEventTag == EventData.EventTag && Row->ActivationTargetId == EventData.TargetId)
		{
			StartQuest(Row->QuestTag);
		}
	}

	ProcessObjectiveProgress(EventData);
}

bool UQuestSubsystem::StartQuest(FGameplayTag QuestTag)
{
	if (ActiveQuests.Contains(QuestTag))
	{
		return false;
	}

	AGYGameState* GameState = GetGYGameState();
	if (GameState && GameState->IsQuestComplete(QuestTag))
	{
		return false;
	}

	if (!ArePrerequisitesMet(QuestTag))
	{
		return false;
	}

	const FQuestTableRow* Row = FindQuestRow(QuestTag);
	if (!Row)
	{
		return false;
	}

	FQuestRuntimeData& RuntimeData = ActiveQuests.Add(QuestTag);
	RuntimeData.QuestTag = QuestTag;
	RuntimeData.State = EQuestState::InProgress;
	RuntimeData.ObjectiveProgress = 0;

	GY_LOG(Content, CYS, "퀘스트 시작: %s", *Row->QuestName.ToString());
	OnQuestStarted.Broadcast(QuestTag);

	return true;
}

void UQuestSubsystem::ProcessObjectiveProgress(const FQuestEventData& EventData)
{
	TArray<FGameplayTag> QuestsToComplete;

	for (auto& Pair : ActiveQuests)
	{
		const FGameplayTag& QuestTag = Pair.Key;
		FQuestRuntimeData& RuntimeData = Pair.Value;

		if (RuntimeData.State != EQuestState::InProgress)
		{
			continue;
		}

		const FQuestTableRow* Row = FindQuestRow(QuestTag);
		if (!Row || !Row->Objective.IsValid())
		{
			continue;
		}

		const FQuestObjective& Objective = Row->Objective;
		if (Objective.ObjectiveEventTag != EventData.EventTag || Objective.TargetId != EventData.TargetId)
		{
			continue;
		}
		if (RuntimeData.ObjectiveProgress >= Objective.RequiredCount)
		{
			continue;
		}

		RuntimeData.ObjectiveProgress = FMath::Min(
			RuntimeData.ObjectiveProgress + EventData.Count,
			Objective.RequiredCount
		);
		OnQuestProgressUpdated.Broadcast(QuestTag, RuntimeData.ObjectiveProgress);

		if (RuntimeData.ObjectiveProgress >= Objective.RequiredCount)
		{
			QuestsToComplete.Add(QuestTag);
		}
	}

	for (const FGameplayTag& QuestTag : QuestsToComplete)
	{
		CompleteQuest(QuestTag);
	}
}

void UQuestSubsystem::CompleteQuest(FGameplayTag QuestTag)
{
	FQuestRuntimeData* RuntimeData = ActiveQuests.Find(QuestTag);
	if (!RuntimeData)
	{
		return;
	}

	RuntimeData->State = EQuestState::Completed;

	AGYGameState* GameState = GetGYGameState();
	if (GameState)
	{
		GameState->AddCompletedQuest(QuestTag);
	}

	const FQuestTableRow* Row = FindQuestRow(QuestTag);
	GY_LOG(Content, CYS, "퀘스트 완료: %s", Row ? *Row->QuestName.ToString() : *QuestTag.ToString());
	OnQuestCompleted.Broadcast(QuestTag);

	ActiveQuests.Remove(QuestTag);
}

const FQuestRuntimeData* UQuestSubsystem::GetQuestRuntimeData(FGameplayTag QuestTag) const
{
	return ActiveQuests.Find(QuestTag);
}

AGYGameState* UQuestSubsystem::GetGYGameState() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWorld* World = GI->GetWorld())
		{
			return World->GetGameState<AGYGameState>();
		}
	}

	return nullptr;
}
