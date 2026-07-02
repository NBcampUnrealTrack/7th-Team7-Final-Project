#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GYQuestEntryWidget.generated.h"

class UButton;
class UCommonTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuestEntrySelected, FGameplayTag /*QuestTag*/);

// 퀘스트 목록 항목의 표시 상태
UENUM(BlueprintType)
enum class EQuestEntryState : uint8
{
	Inactive,	// 비활성 (미시작, 클릭 불가)
	Active,		// 활성 (진행중)
	Completed	// 완료
};

UCLASS()
class GYUI_API UGYQuestEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetQuestData(FGameplayTag InQuestTag, const FText& InQuestName, EQuestEntryState InState);

	FOnQuestEntrySelected OnEntrySelected;

protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

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
