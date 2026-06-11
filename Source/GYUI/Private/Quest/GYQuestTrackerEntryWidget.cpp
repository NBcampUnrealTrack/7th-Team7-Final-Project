#include "Quest/GYQuestTrackerEntryWidget.h"

#include "CommonTextBlock.h"
#include "Components/Button.h"

void UGYQuestTrackerEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UGYQuestTrackerEntryWidget::SetQuestData(FGameplayTag InQuestTag, const FText& InQuestName,
                                              const FText& InObjective)
{
	QuestTag = InQuestTag;
	// 퀘스트명 / 조건
	if (Text_QuestName)
	{
		Text_QuestName->SetText(InQuestName);
	}
	if (Text_Objective)
	{
		Text_Objective->SetText(InObjective);
	}
}
