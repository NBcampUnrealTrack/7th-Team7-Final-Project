// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Interact/GYSkillTreeWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Core/GameplayTags/EventTags.h"
#include "Player/GYPlayerState.h"
#include "SkillTree/GYSkillTreeDataSettings.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "SkillTree/SkillTreeComponent.h"
#include "SkillTree/SkillTreeDataAsset.h"
#include "Widget/Interact/GYSkillConnectionLinesWidget.h"
#include "Widget/Interact/GYSkillNodeWidget.h"


void UGYSkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideTooltip(); // 정보 팝업은 처음엔 숨김

	if (const UGYSkillTreeDataSettings* SkillTreeDataSettings = GetDefault<UGYSkillTreeDataSettings>())
	{
		SkillTreeData = SkillTreeDataSettings->SkillTreeDataAsset.LoadSynchronous();
	}

	if (SkillTreeData)
	{
		RebuildTree();
	}

	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		if (USkillTreeComponent* SkillTree = PS->GetSkillTreeComponent())
		{
			SkillTree->OnSkillTreeChanged.AddUObject(this, &UGYSkillTreeWidget::Refresh);
		}
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		SkillPointHandle = ASC->GetGameplayAttributeValueChangeDelegate(
			UGYProgressionAttributeSet::GetSkillPointAttribute())
			.AddLambda([this](const FOnAttributeChangeData&)
			{
				Refresh();
			});
	}

	UpdateSkillPointText();

	// 스크롤 범위를 아는 첫 Tick 에 중앙 정렬
	bPendingCenter = bCenterOnOpen;

	if (CloseButton && !CloseButton->OnClicked.IsAlreadyBound(this, &ThisClass::OnCloseButtonClicked))
	{
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::OnCloseButtonClicked);
	}

	if (ResetButton && !ResetButton->OnClicked.IsAlreadyBound(this, &ThisClass::OnResetButtonClicked))
	{
		ResetButton->OnClicked.AddDynamic(this, &ThisClass::OnResetButtonClicked);
	}
}

void UGYSkillTreeWidget::NativeDestruct()
{
	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		if (USkillTreeComponent* SkillTree = PS->GetSkillTreeComponent())
		{
			SkillTree->OnSkillTreeChanged.RemoveAll(this);
		}
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			   UGYProgressionAttributeSet::GetSkillPointAttribute())
		   .Remove(SkillPointHandle);
	}
	Super::NativeDestruct();
}

void UGYSkillTreeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bPendingCenter) // 처음 한 번 - 전체 노드를 화면 중앙으로 스크롤
	{
		FVector2D ViewSize = MyGeometry.GetLocalSize();
		if (SkillTreeScrollV)
		{
			const FVector2D ScrollSize = SkillTreeScrollV->GetCachedGeometry().GetLocalSize();
			if (ScrollSize.X > 1.f && ScrollSize.Y > 1.f)
			{
				ViewSize = ScrollSize;
			}
		}

		if (ViewSize.X > 1.f && ViewSize.Y > 1.f)
		{
			CenterOnNodes(ViewSize);
			bPendingCenter = false;
		}
	}

	// 팝업이 떠 있는 동안 마우스를 따라다니게
	if (HoveredNodeWidget && SkillTooltip && SkillTooltip->GetVisibility() != ESlateVisibility::Collapsed)
	{
		UpdateTooltipPosition();
	}
}

int32 UGYSkillTreeWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                      int32 LayerId,
                                      const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

void UGYSkillTreeWidget::PaintConnections(const FGeometry& LinesGeo, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	if (!SkillTreeData) return;
	const FPaintGeometry PaintGeo = LinesGeo.ToPaintGeometry();

	auto GetNode = [&](USkillNodeDataAsset* Node, FVector2D& OutCenter, FVector2D& OutHalf, ESkillNodeState& OutState) -> bool
	{
		const FVector2D* Pos = SkillTreeData->SkillNodePositions.Find(Node->GetFName());
		if (!Pos) return false;
		OutCenter = *Pos;
		OutState = ESkillNodeState::Locked;
		OutHalf  = FVector2D(30.f, 30.f); // 희망크기 못 구할 때 대비 기본값
		if (const TObjectPtr<UGYSkillNodeWidget>* WidgetPtr = NodeWidgets.Find(Node))
		{
			if (*WidgetPtr)
			{
				OutState = (*WidgetPtr)->GetNodeState();
				const FVector2D Desired = (*WidgetPtr)->GetDesiredSize();
				if (Desired.X > 1.f && Desired.Y > 1.f)
				{
					OutHalf = Desired * 0.5f;
				}
			}
		}
		return true;
	};

	auto EdgeDist = [](const FVector2D& Half, const FVector2D& D) -> float
	{
		const float ax = FMath::Abs(D.X);
		const float ay = FMath::Abs(D.Y);
		const float tx = ax > KINDA_SMALL_NUMBER ? Half.X / ax : TNumericLimits<float>::Max();
		const float ty = ay > KINDA_SMALL_NUMBER ? Half.Y / ay : TNumericLimits<float>::Max();
		return FMath::Min(tx, ty);
	};

	for (const auto& Pair : NodeWidgets)
	{
		USkillNodeDataAsset* Node = Pair.Key;
		if (!Node) continue;

		FVector2D CA, HA;
		ESkillNodeState SState;
		if (!GetNode(Node, CA, HA, SState)) continue;

		for (USkillNodeDataAsset* Child : Node->Children)
		{
			if (!Child) continue;

			FVector2D CB, HB;
			ESkillNodeState CState;
			if (!GetNode(Child, CB, HB, CState)) continue;

			const FVector2D Dir = (CB - CA).GetSafeNormal();
			if (Dir.IsNearlyZero()) continue;

			// 각 노드 테두리 + 간격까지 줄인 끝점
			const FVector2D Start = CA + Dir * (EdgeDist(HA, Dir) + ConnectionEdgeGap);
			const FVector2D End   = CB - Dir * (EdgeDist(HB, Dir) + ConnectionEdgeGap);
			if (FVector2D::DotProduct(End - Start, Dir) <= 0.f) continue;

			FLinearColor LineColor = ConnectionColorLocked;
			float Thickness = ConnectionThickness;
			if (SState == ESkillNodeState::Unlocked && CState == ESkillNodeState::Unlocked)
			{
				LineColor = ConnectionColorUnlocked;
			}
			else if (SState == ESkillNodeState::Unlocked && CState == ESkillNodeState::Unlockable)
			{
				LineColor = ConnectionColorAvailable;
				Thickness = ConnectionThicknessHighlighted;
			}

			TArray<FVector2D> Points = { Start, End };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				PaintGeo,
				Points,
				ESlateDrawEffect::None,
				LineColor,
				true,
				Thickness);
		}
	}
}

void UGYSkillTreeWidget::RebuildTree()
{
	if (!SkillTreeCanvas) return;
	SkillTreeCanvas->ClearChildren();
	NodeWidgets.Empty();
	ConnectionLines = nullptr;

	if (!SkillTreeData) return;

	// 연결선 위젯을 먼저 추가
	ConnectionLines = NewObject<UGYSkillConnectionLinesWidget>(this);
	ConnectionLines->SetPaintDelegate(FGYSkillLinesPaint::CreateUObject(this, &UGYSkillTreeWidget::PaintConnections));
	if (UCanvasPanelSlot* LinesSlot = SkillTreeCanvas->AddChildToCanvas(ConnectionLines))
	{
		LinesSlot->SetAnchors(FAnchors(0.f, 0.f));
		LinesSlot->SetAlignment(FVector2D(0.f, 0.f));
		LinesSlot->SetPosition(FVector2D::ZeroVector);
		LinesSlot->SetAutoSize(false);
	}

	TSet<USkillNodeDataAsset*> AllNodes;
	for (USkillNodeDataAsset* Root : SkillTreeData->RootNodes)
	{
		CollectAllNodes(Root, AllNodes);
	}

	for (USkillNodeDataAsset* Node : AllNodes)
	{
		CreateNodeWidget(Node);
	}

	Refresh();
	UpdateContentSize();
	bPendingCenter = bCenterOnOpen;
}

void UGYSkillTreeWidget::CollectAllNodes(USkillNodeDataAsset* RootNode, TSet<USkillNodeDataAsset*>& Collected)
{
	if (!RootNode || Collected.Contains(RootNode)) return;
	Collected.Add(RootNode);
	for (USkillNodeDataAsset* Child : RootNode->Children)
	{
		CollectAllNodes(Child, Collected);
	}
}

void UGYSkillTreeWidget::CreateNodeWidget(USkillNodeDataAsset* Node)
{
	if (!Node || !SkillNodeWidgetClass || !SkillTreeCanvas) return;

	UGYSkillNodeWidget* NodeWidget = CreateWidget<UGYSkillNodeWidget>(this, SkillNodeWidgetClass);
	if (!NodeWidget) return;

	NodeWidget->SetNodeData(Node);
	NodeWidget->SetNodeType(ResolveNodeType(Node));
	NodeWidget->OnNodeClicked.AddDynamic(this, &UGYSkillTreeWidget::HandleNodeClicked);
	NodeWidget->OnNodeHovered.AddDynamic(this, &UGYSkillTreeWidget::HandleNodeHovered);
	NodeWidget->OnNodeUnhovered.AddDynamic(this, &UGYSkillTreeWidget::HandleNodeUnhovered);

	UCanvasPanelSlot* CanvasPanelSlot = SkillTreeCanvas->AddChildToCanvas(NodeWidget);
	if (CanvasPanelSlot && SkillTreeData)
	{
		CanvasPanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		CanvasPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		if (const FVector2D* Pos = SkillTreeData->SkillNodePositions.Find(Node->GetFName()))
		{
			CanvasPanelSlot->SetPosition(*Pos);
		}
		CanvasPanelSlot->SetAutoSize(true);
	}

	NodeWidgets.Add(Node, NodeWidget);
}

void UGYSkillTreeWidget::Refresh()
{
	for (auto& Pair : NodeWidgets)
	{
		if (Pair.Value)
		{
			Pair.Value->SetNodeState(ComputeNodeState(Pair.Key));
		}
	}
	UpdateSkillPointText();
}

ESkillNodeState UGYSkillTreeWidget::ComputeNodeState(USkillNodeDataAsset* Node) const
{
	if (!Node)
	{
		return ESkillNodeState::Locked;
	}
	if (IsNodeUnlocked(Node))
	{
		return ESkillNodeState::Unlocked;
	}
	for (USkillNodeDataAsset* Prereq : Node->Prerequisites)
	{
		if (!IsNodeUnlocked(Prereq))
		{
			return ESkillNodeState::Locked;
		}
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		const float SkillPoint = ASC->GetNumericAttribute(UGYProgressionAttributeSet::GetSkillPointAttribute());
		if (SkillPoint >= 1.f)
		{
			return ESkillNodeState::Unlockable;
		}
	}
	return ESkillNodeState::Locked;
}

bool UGYSkillTreeWidget::IsNodeUnlocked(USkillNodeDataAsset* Node) const
{
	if (!Node) return false;
	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		if (USkillTreeComponent* SkillTreeComponent = PS->GetSkillTreeComponent())
		{
			return SkillTreeComponent->IsNodeUnlocked(Node);
		}
	}
	return false;
}

ESkillNodeType UGYSkillTreeWidget::ResolveNodeType(USkillNodeDataAsset* Node) const
{
	if (const ESkillNodeType* Found = NodeTypeOverrides.Find(Node))
	{
		return *Found;
	}
	return DefaultNodeType;
}

void UGYSkillTreeWidget::HandleNodeClicked(USkillNodeDataAsset* Node)
{
	if (!Node) return;
	const ESkillNodeState State = ComputeNodeState(Node);
	if (State != ESkillNodeState::Unlockable) return;

	if (UGYAbilitySystemComponent* ASC = GetOwnerASC())
	{
		FGameplayEventData Payload;
		Payload.OptionalObject = Node;

		ASC->Server_SendGameplayEvent(GYGameplayTags::Event_SkillTree_Unlock, Payload);
	}
}

void UGYSkillTreeWidget::HandleNodeHovered(UGYSkillNodeWidget* NodeWidget)
{
	ShowTooltipFor(NodeWidget);
}

void UGYSkillTreeWidget::HandleNodeUnhovered(UGYSkillNodeWidget* NodeWidget)
{
	if (HoveredNodeWidget == NodeWidget)
	{
		HideTooltip();
	}
}

void UGYSkillTreeWidget::UpdateSkillPointText()
{
	if (!SkillPointText) return;

	int32 SkillPoint = 0;
	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		SkillPoint = FMath::FloorToInt(
			ASC->GetNumericAttribute(UGYProgressionAttributeSet::GetSkillPointAttribute()));
	}

	if (SkillPointTextFormat.IsEmpty())
	{
		SkillPointText->SetText(FText::AsNumber(SkillPoint));
	}
	else
	{
		SkillPointText->SetText(FText::Format(SkillPointTextFormat, FText::AsNumber(SkillPoint)));
	}
}

void UGYSkillTreeWidget::ShowTooltipFor(UGYSkillNodeWidget* NodeWidget)
{
	HoveredNodeWidget = NodeWidget;
	if (!SkillTooltip || !NodeWidget) return;

	USkillNodeDataAsset* Node = NodeWidget->GetNodeData();
	if (!Node) return;

	if (TooltipNameText)
	{
		TooltipNameText->SetText(Node->SkillName);
	}
	if (TooltipDescText)
	{
		TooltipDescText->SetText(Node->Description);
	}

	SkillTooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateTooltipPosition();
}

void UGYSkillTreeWidget::HideTooltip()
{
	HoveredNodeWidget = nullptr;
	if (SkillTooltip)
	{
		SkillTooltip->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGYSkillTreeWidget::UpdateTooltipPosition()
{
	if (!SkillTooltip) return;

	UCanvasPanelSlot* TooltipSlot = Cast<UCanvasPanelSlot>(SkillTooltip->Slot);
	if (!TooltipSlot) return;

	APlayerController* PC = GetOwningPlayer();
	if (!IsValid(PC)) return;

	const FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(PC);
	TooltipSlot->SetPosition(MousePos + TooltipMouseOffset);
}

bool UGYSkillTreeWidget::GetNodeBounds(FVector2D& OutMin, FVector2D& OutMax) const
{
	if (!SkillTreeData) return false;

	bool bFirst = true;
	for (const auto& Pair : NodeWidgets)
	{
		if (!Pair.Key) continue;
		const FVector2D* Pos = SkillTreeData->SkillNodePositions.Find(Pair.Key->GetFName());
		if (!Pos) continue;

		if (bFirst)
		{
			OutMin = OutMax = *Pos;
			bFirst = false;
		}
		else
		{
			OutMin.X = FMath::Min(OutMin.X, Pos->X);
			OutMin.Y = FMath::Min(OutMin.Y, Pos->Y);
			OutMax.X = FMath::Max(OutMax.X, Pos->X);
			OutMax.Y = FMath::Max(OutMax.Y, Pos->Y);
		}
	}
	return !bFirst;
}

void UGYSkillTreeWidget::UpdateContentSize()
{
	FVector2D Min, Max;
	if (!GetNodeBounds(Min, Max)) return;

	const FVector2D ContentSize(Max.X + ContentPadding, Max.Y + ContentPadding);

	if (SkillTreeCanvasSizeBox)
	{
		SkillTreeCanvasSizeBox->SetWidthOverride(ContentSize.X);
		SkillTreeCanvasSizeBox->SetHeightOverride(ContentSize.Y);
	}
	if (ConnectionLines)
	{
		if (UCanvasPanelSlot* LinesSlot = Cast<UCanvasPanelSlot>(ConnectionLines->Slot))
		{
			LinesSlot->SetSize(ContentSize);
		}
	}
}

void UGYSkillTreeWidget::ApplyScrollOffsets()
{
	if (SkillTreeScrollH)
	{
		PanTarget.X = FMath::Clamp(PanTarget.X, 0.f, SkillTreeScrollH->GetScrollOffsetOfEnd());
		SkillTreeScrollH->SetScrollOffset(PanTarget.X);
	}
	if (SkillTreeScrollV)
	{
		PanTarget.Y = FMath::Clamp(PanTarget.Y, 0.f, SkillTreeScrollV->GetScrollOffsetOfEnd());
		SkillTreeScrollV->SetScrollOffset(PanTarget.Y);
	}
}

void UGYSkillTreeWidget::CenterOnNodes(const FVector2D& ViewSize)
{
	FVector2D Min, Max;
	if (!GetNodeBounds(Min, Max)) return;

	const FVector2D Center = (Min + Max) * 0.5f;

	PanTarget = Center - ViewSize * 0.5f;
	PanTarget.X = FMath::Max(0.f, PanTarget.X);
	PanTarget.Y = FMath::Max(0.f, PanTarget.Y);

	if (SkillTreeScrollH) SkillTreeScrollH->SetScrollOffset(PanTarget.X);
	if (SkillTreeScrollV) SkillTreeScrollV->SetScrollOffset(PanTarget.Y);
}

void UGYSkillTreeWidget::OnCloseButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;

	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_SkillTree_Exit, FGameplayEventData());
}

void UGYSkillTreeWidget::OnResetButtonClicked()
{
	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		if (USkillTreeComponent* SkillTree = PS->GetSkillTreeComponent())
		{
			SkillTree->ServerResetSkillTree();
		}
	}
}

UGYAbilitySystemComponent* UGYSkillTreeWidget::GetOwnerASC() const
{
	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		return PS->GetGYAbilitySystemComponent();
	}
	return nullptr;
}




FReply UGYSkillTreeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton ||
		InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bIsPanning = true;
		LastMouseScreenPos = InMouseEvent.GetScreenSpacePosition();

		// 현재 스크롤 위치에서 드래그 시작
		PanTarget.X = SkillTreeScrollH ? SkillTreeScrollH->GetScrollOffset() : 0.f;
		PanTarget.Y = SkillTreeScrollV ? SkillTreeScrollV->GetScrollOffset() : 0.f;

		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGYSkillTreeWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsPanning &&
		(InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton ||
		 InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton))
	{
		bIsPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UGYSkillTreeWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsPanning)
	{
		const FVector2D Screen = InMouseEvent.GetScreenSpacePosition();
		const FVector2D Delta = (Screen - LastMouseScreenPos) / FMath::Max(InGeometry.Scale, KINDA_SMALL_NUMBER);
		LastMouseScreenPos = Screen;

		// 커서 이동 반대 방향으로 스크롤
		PanTarget -= Delta * PanSpeed;
		ApplyScrollOffsets();
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UGYSkillTreeWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
}
