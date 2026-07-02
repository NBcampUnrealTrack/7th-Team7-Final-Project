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

void UGYQuestEntryWidget::SetQuestData(FGameplayTag InQuestTag, const FText& InQuestName, EQuestEntryState InState)
{
	QuestTag = InQuestTag;
	if (Text_QuestName)
	{
		switch (InState)
		{
		case EQuestEntryState::Completed:
			Text_QuestName->SetText(FText::Format(LOCTEXT("QuestCompletedPrefix", "[완료] {0}"), InQuestName));
			Text_QuestName->SetColorAndOpacity(CompletedTextColor);
			break;
		case EQuestEntryState::Active:
			Text_QuestName->SetText(FText::Format(LOCTEXT("QuestActivePrefix", "[활성] {0}"), InQuestName));
			Text_QuestName->SetColorAndOpacity(DefaultTextColor);
			break;
		case EQuestEntryState::Inactive:
		default:
			Text_QuestName->SetText(InQuestName);
			Text_QuestName->SetColorAndOpacity(DefaultTextColor);
			break;
		}
	}

	if (Button_Quest)
	{
		// 비활성(미시작) 퀘스트는 클릭해서 열 수 없도록 막음
		Button_Quest->SetIsEnabled(InState != EQuestEntryState::Inactive);
	}
}

void UGYQuestEntryWidget::HandleButtonClicked()
{
	OnEntrySelected.Broadcast(QuestTag);
}

#undef LOCTEXT_NAMESPACE
