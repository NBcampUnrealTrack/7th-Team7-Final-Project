#include "Widget/Interact/GYSkillNodeWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "SkillTree/SkillNodeDataAsset.h"

void UGYSkillNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (NodeButton)
	{
		if (!NodeButton->OnClicked.IsAlreadyBound(this, &UGYSkillNodeWidget::HandleNodeClicked))
		{
			NodeButton->OnClicked.AddDynamic(this, &UGYSkillNodeWidget::HandleNodeClicked);
		}
		if (!NodeButton->OnHovered.IsAlreadyBound(this, &UGYSkillNodeWidget::HandleNodeHovered))
		{
			NodeButton->OnHovered.AddDynamic(this, &UGYSkillNodeWidget::HandleNodeHovered);
		}
		if (!NodeButton->OnUnhovered.IsAlreadyBound(this, &UGYSkillNodeWidget::HandleNodeUnhovered))
		{
			NodeButton->OnUnhovered.AddDynamic(this, &UGYSkillNodeWidget::HandleNodeUnhovered);
		}
	}
	ApplyVisuals();
}

void UGYSkillNodeWidget::SetNodeData(USkillNodeDataAsset* InNodeData)
{
	NodeData = InNodeData;
	ApplyVisuals();
	OnNodeDataChanged(NodeData);
}

void UGYSkillNodeWidget::SetNodeState(ESkillNodeState InState)
{
	if (NodeState == InState) return;

	NodeState = InState;
	ApplyVisuals();
	OnNodeStateChanged(NodeState);
}

void UGYSkillNodeWidget::SetNodeType(ESkillNodeType InType)
{
	if (NodeType == InType) return;

	NodeType = InType;
	ApplyVisuals();
	OnNodeTypeChanged(NodeType);
}

void UGYSkillNodeWidget::HandleNodeClicked()
{
	if (NodeData == nullptr) return;

	OnNodeClicked.Broadcast(NodeData);
}

void UGYSkillNodeWidget::HandleNodeHovered()
{
	OnNodeHovered.Broadcast(this);
}

void UGYSkillNodeWidget::HandleNodeUnhovered()
{
	OnNodeUnhovered.Broadcast(this);
}

FLinearColor UGYSkillNodeWidget::ResolveBackgroundColor() const
{
	FLinearColor Base = (NodeType == ESkillNodeType::ActionMechanic) ? ActionMechanicColor : PrimaryStatColor;

	if (NodeState == ESkillNodeState::Locked)
	{
		Base *= LockedTint;
		Base.A = 1.0f;
	}
	return Base;
}

void UGYSkillNodeWidget::ApplyVisuals()
{
	const bool bLocked = (NodeState == ESkillNodeState::Locked);
	const bool bUnlockable = (NodeState == ESkillNodeState::Unlockable);
	const bool bUnlocked = (NodeState == ESkillNodeState::Unlocked);

	// 아이콘 - 잠금이면 아이콘도 어둡게
	if (IconImage)
	{
		if (NodeData && !NodeData->Icon.IsNull())
		{
			IconImage->SetBrushFromSoftTexture(NodeData->Icon, false);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		}
		IconImage->SetColorAndOpacity(bLocked ? LockedTint : FLinearColor::White);
	}

	// 배경색 - 타입 구분 + 잠금 시 어둡게
	if (NodeBackground)
	{
		NodeBackground->SetColorAndOpacity(ResolveBackgroundColor());
		NodeBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 상태별 불투명도
	const float Opacity = bLocked ? LockedOpacity : (bUnlockable ? UnlockableOpacity : UnlockedOpacity);
	SetRenderOpacity(Opacity);

	// 상태 선
	if (SelectionBorder)
	{
		if (bUnlocked)
		{
			SelectionBorder->SetColorAndOpacity(UnlockedRingColor);
			SelectionBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else if (bUnlockable)
		{
			SelectionBorder->SetColorAndOpacity(UnlockableRingColor);
			SelectionBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			SelectionBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
