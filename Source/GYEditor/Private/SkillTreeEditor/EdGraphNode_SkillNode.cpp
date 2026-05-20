// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeEditor/EdGraphNode_SkillNode.h"
#include "SkillTree/SkillNodeDataAsset.h"

const FName UEdGraphNode_SkillNode::PinCategory = TEXT("SkillNode");

void UEdGraphNode_SkillNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, PinCategory, TEXT("In"));
	CreatePin(EGPD_Output, PinCategory, TEXT("Out"));
}

FText UEdGraphNode_SkillNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (SkillAsset)
	{
		return SkillAsset->SkillName.IsEmpty()
			       ? FText::FromName(SkillAsset->GetFName())
			       : SkillAsset->SkillName;
	}
	return NSLOCTEXT("SkillTree", "EmptyNode", "(비어있음)");
}

FText UEdGraphNode_SkillNode::GetTooltipText() const
{
	if (SkillAsset)
	{
		return FText::Format(
			NSLOCTEXT("SkillTree", "Tooltip", "{0}\n{1}"),
			SkillAsset->SkillName, SkillAsset->Description);
	}
	return FText::GetEmpty();
}

FLinearColor UEdGraphNode_SkillNode::GetNodeTitleColor() const
{
	if (!SkillAsset)
	{
		return FLinearColor(0.15f, 0.15f, 0.18f);
	}
	return FLinearColor(0.25f, 0.25f, 0.3f);
}

void UEdGraphNode_SkillNode::PostPasteNode()
{
	Super::PostPasteNode();
	AllocateDefaultPins();
}

UEdGraphPin* UEdGraphNode_SkillNode::GetInputPin() const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			return Pin;
		}
	}
	return nullptr;
}

UEdGraphPin* UEdGraphNode_SkillNode::GetOutputPin() const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			return Pin;
		}
	}
	return nullptr;
}
