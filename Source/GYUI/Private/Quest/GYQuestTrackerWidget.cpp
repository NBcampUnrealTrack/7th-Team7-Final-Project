#include "Quest/GYQuestTrackerWidget.h"

#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"
#include "Quest/GYQuestTrackerEntryWidget.h"
#include "Quest/QuestSubsystem.h"
#include "Quest/QuestTypes.h"

void UGYQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->OnQuestStarted.AddUObject(this, &ThisClass::HandleQuestStarted);
		QS->OnQuestProgressUpdated.AddUObject(this, &ThisClass::HandleQuestProgressUpdated);
		QS->OnQuestCompleted.AddUObject(this, &ThisClass::HandleQuestCompleted);
	}

	RefreshWidget();
}

void UGYQuestTrackerWidget::NativeDestruct()
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->OnQuestStarted.RemoveAll(this);
		QS->OnQuestProgressUpdated.RemoveAll(this);
		QS->OnQuestCompleted.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UGYQuestTrackerWidget::RefreshWidget()
{
	UQuestSubsystem* QS = GetQuestSubsystem();
	if (!QS)
	{
		GY_WARN(Content, CYS, "RefreshWidget - QuestSubsystem NULL");
		return;
	}

	// 진행도 바 업데이트
	const TMap<FGameplayTag, const FQuestTableRow*>& AllRows = QS->GetAllQuestRows();
	const int32 TotalCount = AllRows.Num();
	const AGYGameState* GS = GetGYGameState();
	const int32 CompletedCount = GS ? GS->GetCompletedQuests().Num() : 0;

	const float OverallPercent = TotalCount > 0 ? static_cast<float>(CompletedCount) / TotalCount : 0.f;
	if (Bar_QuestProgress)
	{
		Bar_QuestProgress->SetPercent(OverallPercent);
	}

	// 활성 퀘스트 목록 갱신
	if (!Box_ActiveQuests || !ActiveQuestEntryClass)
	{
		GY_WARN(Content, CYS, "RefreshWidget - Box_ActiveQuests=%s EntryClass=%s",
			Box_ActiveQuests ? TEXT("OK") : TEXT("NULL"),
			ActiveQuestEntryClass ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	Box_ActiveQuests->ClearChildren();

	const TMap<FGameplayTag, FQuestRuntimeData>& ActiveQuests = QS->GetActiveQuests();
	for (const auto& Pair : ActiveQuests)
	{
		const FQuestTableRow* Row = QS->FindQuestRow(Pair.Key);
		if (!Row) continue;

		UGYQuestTrackerEntryWidget* Entry = CreateWidget<UGYQuestTrackerEntryWidget>(this, ActiveQuestEntryClass);
		if (!Entry) continue;

		Entry->SetQuestData(Pair.Key, Row->QuestName, Row->Objective.Description);
		Box_ActiveQuests->AddChild(Entry);
	}

	GY_LOG(Content, CYS, "RefreshWidget - 활성 퀘스트=%d 전체=%d/%d", ActiveQuests.Num(), CompletedCount, TotalCount);
}

void UGYQuestTrackerWidget::HandleQuestStarted(FGameplayTag QuestTag)
{
	RefreshWidget();
}

void UGYQuestTrackerWidget::HandleQuestProgressUpdated(FGameplayTag QuestTag, int32 NewCount)
{
	RefreshWidget();
}

void UGYQuestTrackerWidget::HandleQuestCompleted(FGameplayTag QuestTag)
{
	RefreshWidget();
}

UQuestSubsystem* UGYQuestTrackerWidget::GetQuestSubsystem() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UQuestSubsystem>();
	}
	return nullptr;
}

AGYGameState* UGYQuestTrackerWidget::GetGYGameState() const
{
	return GetWorld() ? Cast<AGYGameState>(GetWorld()->GetGameState()) : nullptr;
}