#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GYQuestEntryWidget.generated.h"

class UButton;
class UCommonTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuestEntrySelected, FGameplayTag /*QuestTag*/);

UCLASS()
class GYUI_API UGYQuestEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetQuestData(FGameplayTag InQuestTag, const FText& InQuestName, bool bCompleted);

	FOnQuestEntrySelected OnEntrySelected;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Quest;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_QuestName;

	// 완료된 퀘스트 텍스트 색상
	UPROPERTY(EditDefaultsOnly, Category = "GY|Quest")
	FLinearColor CompletedTextColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);

	// 미완료 퀘스트 텍스트 색상
	UPROPERTY(EditDefaultsOnly, Category = "GY|Quest")
	FLinearColor DefaultTextColor = FLinearColor::White;

private:
	FGameplayTag QuestTag;

	UFUNCTION()
	void HandleButtonClicked();
};