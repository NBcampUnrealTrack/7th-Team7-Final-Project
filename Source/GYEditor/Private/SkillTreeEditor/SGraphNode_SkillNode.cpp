#include "SkillTreeEditor/SGraphNode_SkillNode.h"
#include "SkillTreeEditor/EdGraphNode_SkillNode.h"
#include "SkillTreeEditor/SGraphPin_SkillPin.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "SGraphPanel.h"
#include "Styling/AppStyle.h"

void SGraphNode_SkillNode::Construct(
	const FArguments& InArgs, UEdGraphNode_SkillNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SGraphNode_SkillNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	const float TotalSize = NodeDiameter + PinRadius * 2.f;

	GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(TotalSize)
			.HeightOverride(TotalSize)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Top)
				.Padding(0.f)
				[
					SAssignNew(InputPinBox, SVerticalBox)
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 0.f, 0.f)
				[
					SAssignNew(IconWidget, SImage)
					.Image(this, &SGraphNode_SkillNode::GetSkillIcon)
					.DesiredSizeOverride(FVector2D(NodeDiameter * 0.85f, NodeDiameter * 0.85f))
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Bottom)
				.Padding(0.f, 0.f, 0.f, -18.f)
				[
					SAssignNew(TitleWidget, STextBlock)
					.Text(this, &SGraphNode_SkillNode::GetSkillTitle)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FLinearColor::White)
					.Justification(ETextJustify::Center)
					.WrapTextAt(NodeDiameter - 8.f)
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Bottom)
				.Padding(0.f)
				[
					SAssignNew(OutputPinBox, SVerticalBox)
				]
			]
		];

	CreatePinWidgets();
}

void SGraphNode_SkillNode::CreatePinWidgets()
{
	UEdGraphNode_SkillNode* SkillNode = CastChecked<UEdGraphNode_SkillNode>(GraphNode);
	for (UEdGraphPin* Pin : SkillNode->Pins)
	{
		AddPin(SNew(SGraphPin_SkillPin, Pin));
	}
}

void SGraphNode_SkillNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));
	if (PinToAdd->GetDirection() == EGPD_Input)
	{
		InputPinBox->AddSlot().AutoHeight().HAlign(HAlign_Center)[PinToAdd];
		InputPins.Add(PinToAdd);
	}
	else
	{
		OutputPinBox->AddSlot().AutoHeight().HAlign(HAlign_Center)[PinToAdd];
		OutputPins.Add(PinToAdd);
	}
}

FVector2D SGraphNode_SkillNode::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(NodeDiameter + PinRadius * 2.f, NodeDiameter + PinRadius * 2.f);
}

int32 SGraphNode_SkillNode::OnPaint(
	const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;

	bool bSelected = false;
	if (TSharedPtr<SGraphPanel> Panel = GetOwnerPanel())
	{
		bSelected = Panel->SelectionManager.IsNodeSelected(GraphNode);
	}
	if (bSelected)
	{
		DrawSelectionRing(OutDrawElements, LayerId, AllottedGeometry, Center, NodeRadius);
	}
	DrawCircle(OutDrawElements, LayerId + 1, AllottedGeometry,
	           Center, NodeRadius + 3.f, FLinearColor(0, 0, 0, 0.35f), 24);

	DrawCircle(OutDrawElements, LayerId + 2, AllottedGeometry,
	           Center, NodeRadius, GetNodeBodyColor().GetSpecifiedColor());

	DrawCircleOutline(OutDrawElements, LayerId + 3, AllottedGeometry,
	                  Center, NodeRadius,
	                  bSelected ? FLinearColor(1.f, 0.85f, 0.1f) : FLinearColor(1, 1, 1, 0.2f),
	                  bSelected ? 2.5f : 1.5f);

	return SGraphNode::OnPaint(Args, AllottedGeometry, MyCullingRect,
	                           OutDrawElements, LayerId + 4, InWidgetStyle, bParentEnabled);
}

FReply SGraphNode_SkillNode::OnMouseButtonDown(
	const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D Center = MyGeometry.GetLocalSize() * 0.5f;
	if (FVector2D::DistSquared(Local, Center) > NodeRadius * NodeRadius)
	{
		return FReply::Unhandled();
	}
	return SGraphNode::OnMouseButtonDown(MyGeometry, MouseEvent);
}

void SGraphNode_SkillNode::DrawCircle(
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FGeometry& Geometry, const FVector2D& Center,
	float Radius, const FLinearColor& Color, int32 Segments) const
{
	TArray<FSlateVertex> Verts;
	TArray<SlateIndex> Indices;
	const FSlateRenderTransform& T = Geometry.GetAccumulatedRenderTransform();
	const FColor C32 = Color.ToFColor(true);
	Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
		T,
		FVector2f(Center),
		FVector2f::ZeroVector,
		C32));

	for (int32 i = 0; i <= Segments; ++i)
	{
		const float A = (2.f * PI * i) / Segments;
		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			T,
			FVector2f(Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius),
			FVector2f::ZeroVector, C32));
	}
	for (int32 i = 1; i <= Segments; ++i)
	{
		Indices.Add(0);
		Indices.Add(i);
		Indices.Add(i % Segments + 1);
	}

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements, LayerId,
		FAppStyle::GetBrush("WhiteTexture")->GetRenderingResource(),
		Verts, Indices, nullptr, 0, 0);
}

void SGraphNode_SkillNode::DrawCircleOutline(
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FGeometry& Geometry, const FVector2D& Center,
	float Radius, const FLinearColor& Color, float Thickness, int32 Segments) const
{
	TArray<FVector2D> Pts;
	for (int32 i = 0; i <= Segments; ++i)
	{
		const float A = (2.f * PI * i) / Segments;
		Pts.Add(Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius);
	}
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(),
		Pts,
		ESlateDrawEffect::None,
		Color,
		true,
		Thickness);
}

void SGraphNode_SkillNode::DrawSelectionRing(
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FGeometry& Geometry, const FVector2D& Center, float Radius) const
{
	for (auto& [R, A] : TArray<TPair<float, float>>{
		     {Radius + 10.f, 0.08f}, {Radius + 6.f, 0.2f}, {Radius + 3.f, 0.6f}
	     })
	{
		DrawCircleOutline(OutDrawElements, LayerId, Geometry,
		                  Center, R, FLinearColor(1.f, 0.85f, 0.1f, A), 2.f);
	}
}

FSlateColor SGraphNode_SkillNode::GetNodeBodyColor() const
{
	const UEdGraphNode_SkillNode* N = Cast<UEdGraphNode_SkillNode>(GraphNode);
	if (!N || !N->SkillAsset)
	{
		return FLinearColor(0.12f, 0.12f, 0.15f);
	}
	return FLinearColor(0.18f, 0.18f, 0.22f);
}

FText SGraphNode_SkillNode::GetSkillTitle() const
{
	const UEdGraphNode_SkillNode* N = Cast<UEdGraphNode_SkillNode>(GraphNode);
	if (N && N->SkillAsset)
	{
		return N->SkillAsset->SkillName.IsEmpty()
			       ? FText::FromName(N->SkillAsset->GetFName())
			       : N->SkillAsset->SkillName;
	}
	return FText::FromString("?");
}

const FSlateBrush* SGraphNode_SkillNode::GetSkillIcon() const
{
	const UEdGraphNode_SkillNode* N = Cast<UEdGraphNode_SkillNode>(GraphNode);
	if (!N || !N->SkillAsset)
	{
		return FAppStyle::GetBrush("ClassIcon.DataAsset");
	}

	UTexture2D* IconTex = N->SkillAsset->Icon.LoadSynchronous();
	if (!IconTex)
	{
		return FAppStyle::GetBrush("ClassIcon.DataAsset");
	}

	constexpr float IconSize = NodeDiameter * 0.85f;

	CachedIconBrush.SetResourceObject(IconTex);
	CachedIconBrush.ImageSize = FVector2D(IconSize, IconSize);
	CachedIconBrush.DrawAs = ESlateBrushDrawType::Image;
	return &CachedIconBrush;
}
