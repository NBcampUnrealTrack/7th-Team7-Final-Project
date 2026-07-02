#include "Quest/QuestSubsystem.h"
#include "Quest/QuestSettings.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/QuestTags.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"
#include "UI/GYUIMessages.h"

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UQuestSettings* Settings = UQuestSettings::Get();
	if (!Settings)
	{
		return;
	}

	CachedQuestTable = Settings->QuestDataTable.LoadSynchronous();
	if (!CachedQuestTable)
	{
		GY_WARN(Content, CYS, "UQuestSubsystem: QuestDataTable이 설정되지 않았습니다.");
		return;
	}

	CachedDialogueTable = Settings->DialogueDataTable.LoadSynchronous();
	if (!CachedDialogueTable)
	{
		GY_WARN(Content, CYS, "UQuestSubsystem: DialogueDataTable이 설정되지 않았습니다.");
	}

	BuildCache(CachedQuestTable);

	Collection.InitializeDependency<UGameplayMessageSubsystem>();
	UGameplayMessageSubsystem* MsgSubsystem = GetGameInstance()->GetSubsystem<UGameplayMessageSubsystem>();
	if (MsgSubsystem)
	{
		QuestStartedListenerHandle = MsgSubsystem->RegisterListener<FGYQuestProgressMessage>(
			GYGameplayTags::Message_Quest_Started,
			this, &UQuestSubsystem::OnQuestStartedFromServer);

		QuestCompletedListenerHandle = MsgSubsystem->RegisterListener<FGYQuestProgressMessage>(
			GYGameplayTags::Message_Quest_Completed,
			this, &UQuestSubsystem::OnQuestCompletedFromServer);

		QuestEventListenerHandle = MsgSubsystem->RegisterListener<FQuestEventMessage>(
			GYGameplayTags::Message_Quest_Event,
			this, &UQuestSubsystem::OnQuestEvent);
	}
}

void UQuestSubsystem::Deinitialize()
{
	QuestStartedListenerHandle.Unregister();
	QuestCompletedListenerHandle.Unregister();
	QuestEventListenerHandle.Unregister();
	Super::Deinitialize();
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
		GY_WARN(Content, CYS, "ArePrerequisitesMet - QuestCache에 없음 (캐시 크기=%d): %s", QuestCache.Num(),
		        *QuestTag.ToString());
		return false;
	}
	GY_LOG(Content, CYS, "ArePrerequisitesMet - PrerequisiteTag=[%s] IsValid=%d", *Row->PrerequisiteTag.ToString(),
	       Row->PrerequisiteTag.IsValid() ? 1 : 0);
	if (!Row->PrerequisiteTag.IsValid())
	{
		return true;
	}
	AGYGameState* GameState = GetGYGameState();
	if (!GameState)
	{
		GY_LOG(Content, CYS, "ArePrerequisitesMet - GameState 없음, 통과 처리");
		return true;
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
		GY_LOG(Content, CYS, "StartQuest 실패 - 이미 진행 중: %s", *QuestTag.ToString());
		return false;
	}

	AGYGameState* GameState = GetGYGameState();
	if (!GameState)
	{
		GY_WARN(Content, CYS, "StartQuest 실패 - GameState 없음: %s", *QuestTag.ToString());
		return false;
	}

	if (GameState->IsQuestComplete(QuestTag))
	{
		GY_LOG(Content, CYS, "StartQuest 실패 - 이미 완료됨: %s", *QuestTag.ToString());
		return false;
	}

	if (!ArePrerequisitesMet(QuestTag))
	{
		GY_LOG(Content, CYS, "StartQuest 실패 - 선행 퀘스트 미완료: %s", *QuestTag.ToString());
		return false;
	}

	const FQuestTableRow* Row = FindQuestRow(QuestTag);
	if (!Row)
	{
		GY_LOG(Content, CYS, "StartQuest 실패 - QuestCache에 없음 (캐시 크기=%d): %s", QuestCache.Num(), *QuestTag.ToString());
		return false;
	}

	GY_LOG(Content, CYS, "퀘스트 시작: %s", *Row->QuestName.ToString());

	GameState->AddActiveQuest(QuestTag);

	MarkQuestStarted(QuestTag);

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
			GY_LOG(Content, CYS, "ProcessObjective - Objective 없음: %s", *QuestTag.ToString());
			continue;
		}

		const FQuestObjective& Objective = Row->Objective;
		GY_LOG(Content, CYS,
		       "ProcessObjective - Quest=%s ObjectiveTag=[%s] EventTag=[%s] ObjTargetId=%s EventTargetId=%s",
		       *QuestTag.ToString(),
		       *Objective.ObjectiveEventTag.ToString(),
		       *EventData.EventTag.ToString(),
		       *Objective.TargetId.ToString(),
		       *EventData.TargetId.ToString());
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
		GameState->RemoveActiveQuest(QuestTag);
	}

	const FQuestTableRow* Row = FindQuestRow(QuestTag);
	GY_LOG(Content, CYS, "퀘스트 완료: %s", Row ? *Row->QuestName.ToString() : *QuestTag.ToString());

	MarkQuestCompleted(QuestTag);
}

const FQuestRuntimeData* UQuestSubsystem::GetQuestRuntimeData(FGameplayTag QuestTag) const
{
	return ActiveQuests.Find(QuestTag);
}

void UQuestSubsystem::BroadcastNarrativeDialogue(FGameplayTag NarrativeTag)
{
	if (!CachedDialogueTable)
	{
		GY_WARN(Content, CYS, "UQuestSubsystem: DialogueDataTable이 로드되지 않았습니다.");
		return;
	}

	TArray<FDialogueRow*> AllRows;
	CachedDialogueTable->GetAllRows<FDialogueRow>(TEXT("UQuestSubsystem::BroadcastNarrativeDialogue"), AllRows);

	TArray<FDialogueRow> Filtered;
	for (const FDialogueRow* Row : AllRows)
	{
		if (Row && Row->NarrativeTag == NarrativeTag)
		{
			Filtered.Add(*Row);
		}
	}

	if (Filtered.IsEmpty())
	{
		GY_WARN(Content, CYS, "UQuestSubsystem: NarrativeTag [%s]에 해당하는 다이얼로그가 없습니다.", *NarrativeTag.ToString());
		return;
	}

	Filtered.Sort([](const FDialogueRow& A, const FDialogueRow& B) { return A.Order < B.Order; });

	GY_LOG(Content, CYS, "다이얼로그 브로드캐스트: [%s] %d줄", *NarrativeTag.ToString(), Filtered.Num());
	OnNarrativeDialogueStarted.Broadcast(Filtered);
}

void UQuestSubsystem::OnQuestStartedFromServer(FGameplayTag Channel, const FGYQuestProgressMessage& Message)
{
	// 클라이언트 전용: ActiveQuests 채우기 + UI 델리게이트 브로드캐스트
	// 서버(호스트)는 StartQuest()에서 이미 처리하므로 중복 방지
	if (ActiveQuests.Contains(Message.QuestId)) return;

	const FQuestTableRow* Row = FindQuestRow(Message.QuestId);
	if (!Row) return;

	MarkQuestStarted(Message.QuestId);
}

void UQuestSubsystem::OnQuestCompletedFromServer(FGameplayTag Channel, const FGYQuestProgressMessage& Message)
{
	// 클라이언트 전용: ActiveQuests 정리 + UI 델리게이트 브로드캐스트
	MarkQuestCompleted(Message.QuestId);
	GY_LOG(Content, CYS, "OnQuestCompletedFromServer - QuestId=%s", *Message.QuestId.ToString());
}

void UQuestSubsystem::MarkQuestStarted(FGameplayTag QuestTag)
{
	FQuestRuntimeData& RuntimeData = ActiveQuests.Add(QuestTag);
	RuntimeData.QuestTag = QuestTag;
	RuntimeData.State = EQuestState::InProgress;
	RuntimeData.ObjectiveProgress = 0;

	OnQuestStarted.Broadcast(QuestTag);
}

void UQuestSubsystem::MarkQuestCompleted(FGameplayTag QuestTag)
{
	ActiveQuests.Remove(QuestTag);
	OnQuestCompleted.Broadcast(QuestTag);
}

void UQuestSubsystem::OnQuestEvent(FGameplayTag Channel, const FQuestEventMessage& Message)
{
	FQuestEventData Data;
	Data.EventTag = Message.EventTag;
	Data.TargetId = Message.TargetId;
	Data.Count = Message.Count;

	HandleQuestEvent(Data);
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
