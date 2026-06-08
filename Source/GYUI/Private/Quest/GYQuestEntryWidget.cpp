#include "Quest/GYQuestEntryWidget.h"

#include "CommonTextBlock.h"
#include "Components/Button.h"

void UGYQuestEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Quest)
	{
		Button_Quest->OnClicked.AddDynamic(this, &ThisClass::HandleButtonClicked);
	}
}

void UGYQuestEntryWidget::SetQuestData(FGameplayTag InQuestTag, const FText& InQuestName, bool bCompleted)
{
	QuestTag = InQuestTag;
	if (Text_QuestName)
	{
		if (bCompleted)
		{ // 완료된 퀘스트
			Text_QuestName->SetText(
				FText::FromString(TEXT("[완료] ") + InQuestName.ToString())
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
