#include "Quest/GYDialogueWidget.h"

#include "CommonTextBlock.h"
#include "Components/TextBlock.h"
#include "Quest/QuestSubsystem.h"

#define LOCTEXT_NAMESPACE "GYUI"

void UGYDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQuestSubsystem* QS = GI->GetSubsystem<UQuestSubsystem>())
		{
			DialogueDelegateHandle = QS->OnNarrativeDialogueStarted.AddUObject(
				this, &UGYDialogueWidget::OnNarrativeDialogueStarted);
		}
	}
}

void UGYDialogueWidget::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(DialogueTimerHandle);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQuestSubsystem* QS = GI->GetSubsystem<UQuestSubsystem>())
		{
			QS->OnNarrativeDialogueStarted.Remove(DialogueDelegateHandle);
		}
	}

	Super::NativeDestruct();
}

void UGYDialogueWidget::OnNarrativeDialogueStarted(TArray<FDialogueRow> Rows)
{
	if (Rows.IsEmpty())
	{
		return;
	}

	DialogueRows = MoveTemp(Rows);
	CurrentIndex = 0;

	SetVisibility(ESlateVisibility::Visible);
	ShowCurrentDialogue();

	GetWorld()->GetTimerManager().SetTimer( //TODO:: 2초마다 넘어가도록 하드코딩 되어 있음
		DialogueTimerHandle, this, &UGYDialogueWidget::AdvanceDialogue, 2.0f, true);
}

void UGYDialogueWidget::ShowCurrentDialogue()
{
	const FDialogueRow& Row = DialogueRows[CurrentIndex];

	const FText SpeakerName = Row.bIsPlayer
		                          ? LOCTEXT("Dialogue_DefaultSpeaker_Player", "플레이어") // TODO:: 플레이어 이름 가져오기
		                          : Row.Speaker;

	SpeakerText->SetText(SpeakerName);
	DialogueText->SetText(Row.Dialogue);
}

void UGYDialogueWidget::AdvanceDialogue()
{
	++CurrentIndex;

	if (!DialogueRows.IsValidIndex(CurrentIndex))
	{
		GetWorld()->GetTimerManager().ClearTimer(DialogueTimerHandle);
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ShowCurrentDialogue();
}

#undef LOCTEXT_NAMESPACE
