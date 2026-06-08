#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "Quest/QuestTypes.h"
#include "GYDialogueWidget.generated.h"

class UCommonTextBlock;
class UTextBlock;
class UQuestSubsystem;

UCLASS()
class GYUI_API UGYDialogueWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeDestruct() override;

protected:
	virtual void NativeConstruct() override;

	// BP에서 바인딩할 텍스트 블록 (이름 일치 필수)
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> SpeakerText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> DialogueText;

private:
	void OnNarrativeDialogueStarted(TArray<FDialogueRow> Rows);
	void ShowCurrentDialogue();
	void AdvanceDialogue();

	TArray<FDialogueRow> DialogueRows;
	int32 CurrentIndex = 0;

	FDelegateHandle DialogueDelegateHandle;
	FTimerHandle DialogueTimerHandle;
};
