#include "Quest/GYQuestEntryWidget.h"

#include "CommonTextBlock.h"
#include "Components/Button.h"

#define LOCTEXT_NAMESPACE "GYUI"

void UGYQuestEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Quest)
	{
		Button_Quest->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleButtonClicked);
	}
}

void UGYQuestEntryWidget::NativeDestruct()
{
	if (Button_Quest)
	{
		Button_Quest->OnClicked.RemoveDynamic(this, &ThisClass::HandleButtonClicked);
	}
	Super::NativeDestruct();
}

void UGYQuestEntryWidget::SetQuestData(FGameplayTag InQuestTag, const FText& InQuestName, bool bCompleted)
{
	QuestTag = InQuestTag;
	if (Text_QuestName)
	{
		if (bCompleted)
		{ // 완료된 퀘스트
			Text_QuestName->SetText(
			FText::Format(LOCTEXT("QuestCompletedPrefix", "[완료] {0}"), InQuestName)
			);
			Text_QuestName->SetColorAndOpacity(CompletedTextColor);
		}
		else
		{ // 진행중 퀘스트
			Text_QuestName->SetText(InQuestName);
			Text_QuestName->SetColorAndOpacity(DefaultTextColor);
		}
		// TODO::비활성 퀘스트 아직 못열게 처리
	}
}

void UGYQuestEntryWidget::HandleButtonClicked()
{
	OnEntrySelected.Broadcast(QuestTag);
}

#undef LOCTEXT_NAMESPACE
