#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "GYQuestListWidget.generated.h"

class UButton;
class UCommonTextBlock;
class UProgressBar;
class UScrollBox;
class UGYQuestEntryWidget;
class UQuestSubsystem;
class AGYGameState;

UCLASS()
class GYUI_API UGYQuestListWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYQuestListWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 왼쪽 패널 - 목록
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_QuestList;

	// 오른쪽 패널 - 상세
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_QuestName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_QuestDesc;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_QuestProgress;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_QuestProgress;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Quest")
	TSubclassOf<UGYQuestEntryWidget> QuestEntryWidgetClass;

private:
	FGameplayTag SelectedQuestTag;

	void RefreshList();
	void ShowQuestDetail(FGameplayTag QuestTag);
	void ClearDetail();

	void HandleEntrySelected(FGameplayTag QuestTag);
	void HandleQuestStarted(FGameplayTag QuestTag);
	void HandleQuestProgressUpdated(FGameplayTag QuestTag, int32 NewCount);
	void HandleQuestCompleted(FGameplayTag QuestTag);

	UQuestSubsystem* GetQuestSubsystem() const;
	AGYGameState* GetGYGameState() const;
};