#include "Quest/QuestSubsystem.h"
#include "Quest/QuestSettings.h"
#include "Engine/DataTable.h"

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
		UE_LOG(LogTemp, Warning, TEXT("UQuestSubsystem: QuestDataTable이 설정되지 않았습니다."));
		return;
	}

	BuildCache(DataTable);
}

void UQuestSubsystem::BuildCache(UDataTable* DataTable)
{
	QuestCache.Empty();

	for (const FName& RowName : DataTable->GetRowNames())
	{
		FQuestTableRow* Row = DataTable->FindRow<FQuestTableRow>(RowName, TEXT("UQuestSubsystem::BuildCache"));
		if (Row)
		{
			QuestCache.Add(RowName, Row);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UQuestSubsystem: 퀘스트 %d개 로드됨"), QuestCache.Num());
}

const FQuestTableRow* UQuestSubsystem::FindQuestRow(FName QuestId) const
{
	const FQuestTableRow* const* Found = QuestCache.Find(QuestId);
	return Found ? *Found : nullptr;
}

bool UQuestSubsystem::ArePrerequisitesMet(FName QuestId, const TArray<FQuestRuntimeData>& CompletedQuests) const
{
	const FQuestTableRow* Row = FindQuestRow(QuestId);
	if (!Row)
	{
		return false;
	}

	for (const FName& PrereqId : Row->PrerequisiteIds)
	{
		const bool bCompleted = CompletedQuests.ContainsByPredicate([&PrereqId](const FQuestRuntimeData& Data)
		{
			return Data.QuestId == PrereqId && Data.State == EQuestState::Completed;
		});

		if (!bCompleted)
		{
			return false;
		}
	}

	return true;
}

bool UQuestSubsystem::IsQuestComplete(const FQuestRuntimeData& RuntimeData) const
{
	const FQuestTableRow* Row = FindQuestRow(RuntimeData.QuestId);
	if (!Row)
	{
		return false;
	}

	if (RuntimeData.ObjectiveProgress.Num() != Row->Objectives.Num())
	{
		return false;
	}

	for (int32 i = 0; i < Row->Objectives.Num(); ++i)
	{
		if (RuntimeData.ObjectiveProgress[i] < Row->Objectives[i].RequiredCount)
		{
			return false;
		}
	}

	return true;
}