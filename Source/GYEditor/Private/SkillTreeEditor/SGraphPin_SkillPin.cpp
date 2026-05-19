#include "SkillTreeEditor/SGraphPin_SkillPin.h"
#include "Styling/AppStyle.h"

void SGraphPin_SkillPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
    SGraphPin::Construct(SGraphPin::FArguments(), InPin);
    bShowLabel = false;
}

FSlateColor SGraphPin_SkillPin::GetPinColor() const
{
    if (IsHovered())   return FLinearColor(1.f, 1.f, 0.3f);
    if (IsConnected()) return FLinearColor(0.1f, 0.9f, 0.4f);
    return FLinearColor(0.35f, 0.35f, 0.45f);
}

TSharedRef<SWidget> SGraphPin_SkillPin::GetDefaultValueWidget()
{
    return SNullWidget::NullWidget;
}

FVector2D SGraphPin_SkillPin::ComputeDesiredSize(float) const
{
    return FVector2D(PinRadius * 2.f, PinRadius * 2.f);
}

int32 SGraphPin_SkillPin::OnPaint(
    const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    DrawPinCircle(OutDrawElements, LayerId, AllottedGeometry,
        GetPinColor().GetSpecifiedColor());
    return LayerId + 1;
}

void SGraphPin_SkillPin::DrawPinCircle(
    FSlateWindowElementList& OutDrawElements, int32 LayerId,
    const FGeometry& Geometry, const FLinearColor& Color) const
{
    const FVector2D Center = Geometry.GetLocalSize() * 0.5f;
    const int32 Segments   = 16;
    TArray<FSlateVertex> Verts;
    TArray<SlateIndex>   Indices;
    const FSlateRenderTransform& T = Geometry.GetAccumulatedRenderTransform();
    const FColor C32 = Color.ToFColor(true);

    Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
        T, FVector2f(Center), FVector2f::ZeroVector, C32));

    for (int32 i = 0; i <= Segments; ++i)
    {
        const float A = (2.f * PI * i) / Segments;
        Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(T,
            FVector2f(Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * PinRadius),
            FVector2f::ZeroVector, C32));
    }
    for (int32 i = 1; i <= Segments; ++i)
    {
        Indices.Add(0); Indices.Add(i); Indices.Add(i % Segments + 1);
    }

    FSlateDrawElement::MakeCustomVerts(
        OutDrawElements, LayerId,
        FAppStyle::GetBrush("WhiteTexture")->GetRenderingResource(),
        Verts, Indices, nullptr, 0, 0);

    TArray<FVector2D> Pts;
    for (int32 i = 0; i <= Segments; ++i)
    {
        const float A = (2.f * PI * i) / Segments;
        Pts.Add(Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * PinRadius);
    }
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
        Geometry.ToPaintGeometry(), Pts,
        ESlateDrawEffect::None, FLinearColor(1, 1, 1, 0.4f), true, 1.f);
}
