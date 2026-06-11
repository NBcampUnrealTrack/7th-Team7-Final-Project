#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GameplayTagContainer.h"
#include "GYQuestTrackerWidget.generated.h"

class AGYGameState;
class UQuestSubsystem;
class UExpandableArea;
class UProgressBar;
class UVerticalBox;
class UGYQuestTrackerEntryWidget;

UCLASS()
class GYUI_API UGYQuestTrackerWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UExpandableArea> QuestExpandableArea;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_QuestProgress;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> Box_ActiveQuests;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Quest")
	TSubclassOf<UGYQuestTrackerEntryWidget> ActiveQuestEntryClass;

private:
	void RefreshWidget();
	void HandleQuestStarted(FGameplayTag QuestTag);
	void HandleQuestProgressUpdated(FGameplayTag QuestTag, int32 NewCount);
	void HandleQuestCompleted(FGameplayTag QuestTag);

	UQuestSubsystem* GetQuestSubsystem() const;
	AGYGameState* GetGYGameState() const;
};
