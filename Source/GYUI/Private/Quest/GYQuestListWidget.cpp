#include "Quest/GYQuestListWidget.h"

#include "Quest/GYQuestEntryWidget.h"
#include "CommonTextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Quest/QuestSubsystem.h"
#include "Quest/QuestTypes.h"
#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"

#define LOCTEXT_NAMESPACE "GYUI"

UGYQuestListWidget::UGYQuestListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EGYWidgetInputMode::Menu;
}

void UGYQuestListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GY_LOG(Content, CYS, "NativeConstruct - ScrollBox=%s EntryClass=%s",
		ScrollBox_QuestList ? TEXT("OK") : TEXT("NULL"),
		QuestEntryWidgetClass ? TEXT("OK") : TEXT("NULL"));

	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		GY_LOG(Content, CYS, "NativeConstruct - QuestSubsystem OK, 전체 퀘스트 수=%d", QS->GetAllQuestRows().Num());
		QS->OnQuestStarted.AddUObject(this, &ThisClass::HandleQuestStarted);
		QS->OnQuestProgressUpdated.AddUObject(this, &ThisClass::HandleQuestProgressUpdated);
		QS->OnQuestCompleted.AddUObject(this, &ThisClass::HandleQuestCompleted);
	}
	else
	{
		GY_WARN(Content, CYS, "NativeConstruct - QuestSubsystem NULL");
	}

	RefreshList();
	ClearDetail();
}

void UGYQuestListWidget::NativeDestruct()
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->OnQuestStarted.RemoveAll(this);
		QS->OnQuestProgressUpdated.RemoveAll(this);
		QS->OnQuestCompleted.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UGYQuestListWidget::RefreshList()
{
	if (!ScrollBox_QuestList || !QuestEntryWidgetClass)
	{
		GY_WARN(Content, CYS, "RefreshList - 중단: ScrollBox=%s EntryClass=%s",
			ScrollBox_QuestList ? TEXT("OK") : TEXT("NULL"),
			QuestEntryWidgetClass ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	ScrollBox_QuestList->ClearChildren();

	UQuestSubsystem* QS = GetQuestSubsystem();
	if (!QS)
	{
		GY_WARN(Content, CYS, "RefreshList - QuestSubsystem NULL");
		return;
	}

	AGYGameState* GS = GetGYGameState();
	const TMap<FGameplayTag, const FQuestTableRow*>& AllRows = QS->GetAllQuestRows();
	const int32 TotalCount = AllRows.Num();
	int32 CompletedCount = 0;

	GY_LOG(Content, CYS, "RefreshList - 전체 퀘스트 수=%d", TotalCount);

	for (const auto& Pair : AllRows)
	{
		const FQuestTableRow* Row = Pair.Value;
		if (!Row)
		{
			GY_WARN(Content, CYS, "RefreshList - Row 없음: Tag=%s", *Pair.Key.ToString());
			continue;
		}

		const bool bCompleted = GS && GS->IsQuestComplete(Pair.Key);
		if (bCompleted)
		{
			++CompletedCount;
		}
		const bool bActive = QS->GetActiveQuests().Contains(Pair.Key);
		const EQuestEntryState State = bCompleted ? EQuestEntryState::Completed
			: bActive ? EQuestEntryState::Active
			: EQuestEntryState::Inactive;

		UGYQuestEntryWidget* Entry = CreateWidget<UGYQuestEntryWidget>(this, QuestEntryWidgetClass);
		if (Entry)
		{
			Entry->SetQuestData(Pair.Key, Row->QuestName, State);
			Entry->OnEntrySelected.AddUObject(this, &ThisClass::HandleEntrySelected);
			ScrollBox_QuestList->AddChild(Entry);
			GY_LOG(Content, CYS, "RefreshList - 항목 추가: %s (완료=%s)", *Row->QuestName.ToString(), bCompleted ? TEXT("Y") : TEXT("N"));
		}
	}

	// 전체 퀘스트 진행도 업데이트
	const float OverallPercent = TotalCount > 0 ? static_cast<float>(CompletedCount) / TotalCount : 0.f;
	if (Bar_QuestProgress)
	{
		Bar_QuestProgress->SetPercent(OverallPercent);
	}
	if (Text_QuestProgress)
	{
		Text_QuestProgress->SetText(FText::Format(LOCTEXT("QuestProgressFormat", "{0}/{1}"),
			FText::AsNumber(CompletedCount), FText::AsNumber(TotalCount)));
	}
	GY_LOG(Content, CYS, "RefreshList - 전체 진행도: %d/%d", CompletedCount, TotalCount);
}

void UGYQuestListWidget::ShowQuestDetail(FGameplayTag QuestTag)
{
	SelectedQuestTag = QuestTag;

	GY_LOG(Content, CYS, "ShowQuestDetail - Tag=%s", *QuestTag.ToString());

	UQuestSubsystem* QS = GetQuestSubsystem();
	if (!QS)
	{
		GY_WARN(Content, CYS, "ShowQuestDetail - QuestSubsystem NULL");
		return;
	}

	const FQuestTableRow* Row = QS->FindQuestRow(QuestTag);
	if (!Row)
	{
		GY_WARN(Content, CYS, "ShowQuestDetail - Row 없음: Tag=%s", *QuestTag.ToString());
		return;
	}

	GY_LOG(Content, CYS, "ShowQuestDetail - 이름=%s 목표=%d", *Row->QuestName.ToString(), Row->Objective.RequiredCount);

	if (Text_QuestName)
	{
		Text_QuestName->SetText(Row->QuestName);
	}
	if (Text_QuestDesc)
	{
		Text_QuestDesc->SetText(Row->Description);
	}
	if (Text_Objective)
	{
		Text_Objective->SetText(Row->Objective.Description);
	}

}

void UGYQuestListWidget::ClearDetail()
{
	SelectedQuestTag = FGameplayTag::EmptyTag;

	if (Text_QuestName)
	{
		Text_QuestName->SetText(FText::GetEmpty());
	}
	if (Text_QuestDesc)
	{
		Text_QuestDesc->SetText(FText::GetEmpty());
	}
	if (Text_Objective)
	{
		Text_Objective->SetText(FText::GetEmpty());
	}
}

void UGYQuestListWidget::HandleEntrySelected(FGameplayTag QuestTag)
{
	ShowQuestDetail(QuestTag);
}

void UGYQuestListWidget::HandleQuestStarted(FGameplayTag QuestTag)
{
	RefreshList();
}

void UGYQuestListWidget::HandleQuestProgressUpdated(FGameplayTag QuestTag, int32 NewCount)
{
	if (SelectedQuestTag == QuestTag)
	{
		ShowQuestDetail(QuestTag);
	}
}

void UGYQuestListWidget::HandleQuestCompleted(FGameplayTag QuestTag)
{
	RefreshList();
	if (SelectedQuestTag == QuestTag)
	{
		ShowQuestDetail(QuestTag);
	}
}

UQuestSubsystem* UGYQuestListWidget::GetQuestSubsystem() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UQuestSubsystem>();
	}
	return nullptr;
}

AGYGameState* UGYQuestListWidget::GetGYGameState() const
{
	return GetWorld() ? Cast<AGYGameState>(GetWorld()->GetGameState()) : nullptr;
}

#undef LOCTEXT_NAMESPACE
