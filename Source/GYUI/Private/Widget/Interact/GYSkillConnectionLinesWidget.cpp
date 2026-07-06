#include "Widget/Interact/GYSkillConnectionLinesWidget.h"
#include "Widgets/SLeafWidget.h"

class SGYSkillConnectionLines : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGYSkillConnectionLines) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SetCanTick(false);
	}

	void SetPaintDelegate(const FGYSkillLinesPaint& InDelegate)
	{
		PaintDelegate = InDelegate;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		if (PaintDelegate.IsBound())
		{
			PaintDelegate.Execute(AllottedGeometry, OutDrawElements, LayerId);
		}
		return LayerId;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D::ZeroVector;
	}

private:
	FGYSkillLinesPaint PaintDelegate;
};


void UGYSkillConnectionLinesWidget::SetPaintDelegate(const FGYSkillLinesPaint& InDelegate)
{
	PaintDelegate = InDelegate;
	if (MyLines.IsValid())
	{
		MyLines->SetPaintDelegate(InDelegate);
	}
}

TSharedRef<SWidget> UGYSkillConnectionLinesWidget::RebuildWidget()
{
	MyLines = SNew(SGYSkillConnectionLines);
	MyLines->SetPaintDelegate(PaintDelegate);
	return MyLines.ToSharedRef();
}

void UGYSkillConnectionLinesWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyLines.Reset();
}
