// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Interact/GYSkillNodeWidget.h"

#include "Components/Button.h"

void UGYSkillNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (NodeButton)
	{
		NodeButton->OnClicked.AddDynamic(this, &UGYSkillNodeWidget::HandleNodeClicked);
	}
}

void UGYSkillNodeWidget::SetNodeData(USkillNodeDataAsset* InNodeData)
{
	NodeData = InNodeData;
	OnNodeDataChanged(NodeData);
}

void UGYSkillNodeWidget::SetNodeState(ESkillNodeState InState)
{
	if (NodeState == InState) return;

	NodeState = InState;
	OnNodeStateChanged(NodeState);

}

void UGYSkillNodeWidget::HandleNodeClicked()
{
	if (NodeData == nullptr) return;

	OnNodeClicked.Broadcast(NodeData);
}
