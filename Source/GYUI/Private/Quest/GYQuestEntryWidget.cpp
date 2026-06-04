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
		Text_QuestName->SetText(InQuestName);
		Text_QuestName->SetColorAndOpacity(bCompleted ? CompletedTextColor : DefaultTextColor);
	}
}

void UGYQuestEntryWidget::HandleButtonClicked()
{
	OnEntrySelected.Broadcast(QuestTag);
}