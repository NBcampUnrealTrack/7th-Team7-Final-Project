#include "Quest/GYQuestTrackerWidget.h"

#include "Components/ProgressBar.h"
#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"
#include "Quest/QuestSubsystem.h"

void UGYQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		GY_LOG(Content, CYS, "NativeConstruct - QuestSubsystem OK, 전체 퀘스트 수=%d", QS->GetAllQuestRows().Num());
		QS->OnQuestCompleted.AddUObject(this, &ThisClass::HandleQuestCompleted);
	}
}

void UGYQuestTrackerWidget::RefreshWidget()
{
	UQuestSubsystem* QS = GetQuestSubsystem();
	if (!QS)
	{
		GY_WARN(Content, CYS, "RefreshWidget - QuestSubsystem NULL");
		return;
	}

	const TMap<FGameplayTag, const FQuestTableRow*>& AllRows = QS->GetAllQuestRows();
	const int32 TotalCount = AllRows.Num();

	const AGYGameState* GS = GetGYGameState();
	const int32 CompletedCount = GS ? GS->GetCompletedQuests().Num() : 0;

	GY_LOG(Content, CYS, "RefreshWidget - 전체 퀘스트 수=%d", TotalCount);

	const float OverallPercent = TotalCount > 0 ? static_cast<float>(CompletedCount) / TotalCount : 0.f;
	if (Bar_QuestProgress)
	{
		Bar_QuestProgress->SetPercent(OverallPercent);
	}
	GY_LOG(Content, CYS, "RefreshWidget - 전체 진행도: %d/%d", CompletedCount, TotalCount);
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
