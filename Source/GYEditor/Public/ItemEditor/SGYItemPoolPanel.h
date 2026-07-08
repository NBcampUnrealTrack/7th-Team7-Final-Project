#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class FGYItemEditorController;
class SVerticalBox;
class UDataTable;
class UItemDefinition;
struct FGYPoolRowValues;

// 아이템의 풀 소속(체크박스)과 드랍 파라미터(Weight/MinCount/MaxCount), Region 노출을 한 화면에 보여주는 패널.
class SGYItemPoolPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGYItemPoolPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController);

	void SetItem(UItemDefinition* Item);
	void Refresh();

private:
	TSharedRef<SWidget> MakeCountBox(const FText& Label, TWeakObjectPtr<UDataTable> PoolWeak, int32 FGYPoolRowValues::* Field);

	TSharedPtr<FGYItemEditorController> Controller;
	TWeakObjectPtr<UItemDefinition> CurrentItem;
	TSharedPtr<SVerticalBox> ContentBox;
};
