#pragma once

#include "CoreMinimal.h"
#include "ItemEditor/Validation/GYItemValidationTypes.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;

class SGYItemValidationPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGYItemValidationPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Report가 nullptr면 "아이템을 선택하세요" 상태로 표시
	void SetReport(const FGYItemValidationReport* Report);

private:
	TSharedPtr<SVerticalBox> MessageBox;
};
