#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UEdGraphNode_SkillNode;

class SGraphNode_SkillNode : public SGraphNode
{
public:
    SLATE_BEGIN_ARGS(SGraphNode_SkillNode) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphNode_SkillNode* InNode);

    virtual void UpdateGraphNode() override;
    virtual void CreatePinWidgets() override;
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

    virtual FReply OnMouseButtonDown(
        const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;

protected:
    virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
    virtual bool IsNameReadOnly() const override { return true; }

private:
    void DrawCircle(
        FSlateWindowElementList& OutDrawElements, int32 LayerId,
        const FGeometry& Geometry, const FVector2D& Center,
        float Radius, const FLinearColor& Color, int32 Segments = 32) const;

    void DrawCircleOutline(
        FSlateWindowElementList& OutDrawElements, int32 LayerId,
        const FGeometry& Geometry, const FVector2D& Center,
        float Radius, const FLinearColor& Color,
        float Thickness = 1.5f, int32 Segments = 32) const;

    void DrawSelectionRing(
        FSlateWindowElementList& OutDrawElements, int32 LayerId,
        const FGeometry& Geometry, const FVector2D& Center,
        float Radius) const;

    FSlateColor        GetNodeBodyColor() const;
    FText              GetSkillTitle()    const;
    const FSlateBrush* GetSkillIcon()     const;

    TSharedPtr<SVerticalBox> InputPinBox;
    TSharedPtr<SVerticalBox> OutputPinBox;
    TSharedPtr<SImage>       IconWidget;
    TSharedPtr<STextBlock>   TitleWidget;

    static constexpr float NodeRadius   = 52.f;
    static constexpr float PinRadius    = 7.f;
    static constexpr float NodeDiameter = NodeRadius * 2.f;
};
