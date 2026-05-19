#pragma once

#include "CoreMinimal.h"
#include "SGraphPin.h"

class SGraphPin_SkillPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphPin_SkillPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

protected:
	virtual FSlateColor GetPinColor() const override;
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	virtual FVector2D ComputeDesiredSize(float) const override;

	virtual int32 OnPaint(
		const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	void DrawPinCircle(
		FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& Geometry, const FLinearColor& Color) const;

	static constexpr float PinRadius = 7.f;
};
