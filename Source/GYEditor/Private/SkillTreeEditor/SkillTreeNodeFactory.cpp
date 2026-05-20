#include "SkillTreeEditor/SkillTreeNodeFactory.h"
#include "SkillTreeEditor/EdGraphNode_SkillNode.h"
#include "SkillTreeEditor/SGraphNode_SkillNode.h"
#include "SkillTreeEditor/SGraphPin_SkillPin.h"

TSharedPtr<SGraphNode> FSkillTreeNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UEdGraphNode_SkillNode* SkillNode = Cast<UEdGraphNode_SkillNode>(Node))
	{
		return SNew(SGraphNode_SkillNode, SkillNode);
	}
	return nullptr;
}

TSharedPtr<SGraphPin> FSkillTreePinFactory::CreatePin(UEdGraphPin* Pin) const
{
	if (Pin && Pin->PinType.PinCategory == UEdGraphNode_SkillNode::PinCategory)
	{
		return SNew(SGraphPin_SkillPin, Pin);
	}
	return nullptr;
}
