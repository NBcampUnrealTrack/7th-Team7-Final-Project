#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "GYSkillConnectionLinesWidget.generated.h"

struct FGeometry;
class FSlateWindowElementList;

DECLARE_DELEGATE_ThreeParams(FGYSkillLinesPaint, const FGeometry& /*LinesGeo*/, FSlateWindowElementList& /*OutDrawElements*/, int32 /*LayerId*/);

/**
 * 노드 연결선 전용 위젯
 */
UCLASS()
class GYUI_API UGYSkillConnectionLinesWidget : public UWidget
{
	GENERATED_BODY()

public:
	void SetPaintDelegate(const FGYSkillLinesPaint& InDelegate);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	FGYSkillLinesPaint PaintDelegate;
	TSharedPtr<class SGYSkillConnectionLines> MyLines;
};
