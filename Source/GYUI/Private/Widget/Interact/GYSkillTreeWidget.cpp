// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Interact/GYSkillTreeWidget.h"

#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Core/GameplayTags/EventTags.h"
#include "Player/GYPlayerState.h"
#include "SkillTree/GYSkillTreeDataSettings.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "SkillTree/SkillTreeComponent.h"
#include "SkillTree/SkillTreeDataAsset.h"
#include "Widget/Interact/GYSkillNodeWidget.h"


void UGYSkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();


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
			UGYPlayerAttribute::GetSkillPointAttribute())
			.AddLambda([this](const FOnAttributeChangeData&)
			{
				Refresh();
			});
	}

	if (CloseButton && !CloseButton->OnClicked.IsAlreadyBound(this, &ThisClass::OnCloseButtonClicked))
	{
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::OnCloseButtonClicked);
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
			   UGYPlayerAttribute::GetSkillPointAttribute())
		   .Remove(SkillPointHandle);
	}
	Super::NativeDestruct();
}

int32 UGYSkillTreeWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                      int32 LayerId,
                                      const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
	  OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!SkillTreeCanvas || !SkillTreeData) return BaseLayer;

	const FGeometry& CanvasGeo = SkillTreeCanvas->GetCachedGeometry();

	auto GetNodeLocalCenter = [&](USkillNodeDataAsset* Node, FVector2D& OutLocal) -> bool
	{
		if (!Node) return false;
		const FVector2D* PosPtr = SkillTreeData->SkillNodePositions.Find(Node->GetFName());
		if (!PosPtr) return false;

		const FVector2D Abs = CanvasGeo.LocalToAbsolute(*PosPtr);
		OutLocal = AllottedGeometry.AbsoluteToLocal(Abs);
		return true;
	};

	for (const auto& Pair : NodeWidgets)
	{
		USkillNodeDataAsset* Node = Pair.Key;
		if (!Node) continue;

		FVector2D StartLocal;
		if (!GetNodeLocalCenter(Node, StartLocal)) continue;

		for (USkillNodeDataAsset* Child : Node->Children)
		{
			FVector2D EndLocal;
			if (!GetNodeLocalCenter(Child, EndLocal)) continue;

			TArray<FVector2D> Points = { StartLocal, EndLocal };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				BaseLayer + 1,
				AllottedGeometry.ToPaintGeometry(),
				Points,
				ESlateDrawEffect::None,
				ConnectionColor,
				true,
				ConnectionThickness);
		}
	}

	return BaseLayer + 2;
}

void UGYSkillTreeWidget::RebuildTree()
{
	if (!SkillTreeCanvas) return;
	SkillTreeCanvas->ClearChildren();
	NodeWidgets.Empty();

	if (!SkillTreeData) return;

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
	NodeWidget->OnNodeClicked.AddDynamic(this, &UGYSkillTreeWidget::HandleNodeClicked);

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
		const float SkillPoint = ASC->GetNumericAttribute(UGYPlayerAttribute::GetSkillPointAttribute());
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

void UGYSkillTreeWidget::OnCloseButtonClicked()
{
	AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState());
	if (!PS) return;
	UGYAbilitySystemComponent* ASC = Cast<UGYAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	if (!ASC) return;

	ASC->Server_SendGameplayEvent(GYGameplayTags::Event_TimeRift_SkillTree_Exit, FGameplayEventData());
}

UGYAbilitySystemComponent* UGYSkillTreeWidget::GetOwnerASC() const
{
	if (AGYPlayerState* PS = Cast<AGYPlayerState>(GetOwningPlayerState()))
	{
		return PS->GetGYAbilitySystemComponent();
	}
	return nullptr;
}




FReply UGYSkillTreeWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    const float WheelDelta = InMouseEvent.GetWheelDelta();
    const float OldZoom = CanvasZoom;
    const float NewZoom = FMath::Clamp(CanvasZoom + WheelDelta * ZoomStep, MinZoom, MaxZoom);

    if (!FMath::IsNearlyEqual(OldZoom, NewZoom))
    {
        const FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        const float ZoomRatio = NewZoom / OldZoom;
        CanvasPan = LocalMouse - (LocalMouse - CanvasPan) * ZoomRatio;
        CanvasZoom = NewZoom;
        ApplyCanvasTransform();
    }

    return FReply::Handled();
}

FReply UGYSkillTreeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton ||
        InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
    {
        bIsPanning = true;
        LastMouseScreenPos = InMouseEvent.GetScreenSpacePosition();
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
        const FVector2D Delta = Screen - LastMouseScreenPos;
        LastMouseScreenPos = Screen;

        CanvasPan += Delta / InGeometry.Scale;
        ApplyCanvasTransform();
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UGYSkillTreeWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
}

void UGYSkillTreeWidget::ApplyCanvasTransform()
{
    if (!SkillTreeCanvas) return;
    FWidgetTransform T;
    T.Translation = CanvasPan;
    T.Scale = FVector2D(CanvasZoom, CanvasZoom);
    SkillTreeCanvas->SetRenderTransform(T);
}
