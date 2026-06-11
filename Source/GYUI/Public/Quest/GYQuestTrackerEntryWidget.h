#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GYQuestTrackerEntryWidget.generated.h"

class UButton;
class UCommonTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestTrackerEntryClicked, FGameplayTag, QuestTag);

UCLASS()
class GYUI_API UGYQuestTrackerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetQuestData(FGameplayTag InQuestTag, const FText& InQuestName, const FText& InObjective);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_QuestName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Objective;

private:
	FGameplayTag QuestTag;
};
