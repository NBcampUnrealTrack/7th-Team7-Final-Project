#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class FSkillTreeNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};

class FSkillTreePinFactory : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* Pin) const override;
};
