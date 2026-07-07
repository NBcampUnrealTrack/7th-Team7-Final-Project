#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class FGYItemEditorController;
class SVerticalBox;
class SWidgetSwitcher;
class SWindow;
class UItemDefinition;
struct FGYItemPreset;

// 3단계 생성 위저드: 타입 선택 → 입력 → 요약/생성
class SGYItemCreationWizard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGYItemCreationWizard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController, TSharedRef<SWindow> InWindow);

	// 모달로 열고 생성된 아이템을 반환. 취소하면 nullptr
	static UItemDefinition* ShowModal(TSharedRef<FGYItemEditorController> InController);

private:
	TSharedRef<SWidget> BuildTypeStep();
	TSharedRef<SWidget> BuildNavigation();
	void RebuildInputStep();
	void RebuildSummaryStep();

	void GoToStep(int32 Step);
	FText GetValidationError() const;
	FGameplayTag GetSelectedSlotTag() const;
	FReply HandleCreateClicked();
	void CloseWindow();

	TSharedPtr<FGYItemEditorController> Controller;
	TWeakPtr<SWindow> Window;

	TSharedPtr<SWidgetSwitcher> Switcher;
	TSharedPtr<SVerticalBox> InputBox;
	TSharedPtr<SVerticalBox> SummaryBox;

	int32 CurrentStep = 0;
	const FGYItemPreset* SelectedPreset = nullptr;

	FString NameInput;
	FString ItemIdInput;
	bool bItemIdEdited = false;
	FString DisplayNameInput;
	bool bDisplayNameEdited = false;

	int32 WeaponTypeIndex = 0;
	int32 AccessorySlotIndex = 0;
	float BaseATK = 10.f;
	float BaseDEF = 10.f;
	float BaseHP = 0.f;
	int32 MaxStackSize = 10;
	bool bRegisterToPool = true;

	TArray<TSharedPtr<FString>> WeaponTypeOptions;
	TArray<FGameplayTag> WeaponTypeTags;
	TArray<TSharedPtr<FString>> AccessorySlotOptions;
	TArray<FGameplayTag> AccessorySlotTags;

	TWeakObjectPtr<UItemDefinition> CreatedItem;
};
