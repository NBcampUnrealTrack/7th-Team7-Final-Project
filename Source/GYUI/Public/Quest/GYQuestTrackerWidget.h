#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYQuestTrackerWidget.generated.h"

class AGYGameState;
class UQuestSubsystem;
class UExpandableArea;
class UProgressBar;

UCLASS()
class GYUI_API UGYQuestTrackerWidget : public UGYUserWidget
{
	GENERATED_BODY()

public:
protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UExpandableArea> QuestExpandableArea;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_QuestProgress;

private:
	void RefreshWidget();
	void HandleQuestCompleted(FGameplayTag QuestTag);

	UQuestSubsystem* GetQuestSubsystem() const;
	AGYGameState* GetGYGameState() const;
};
