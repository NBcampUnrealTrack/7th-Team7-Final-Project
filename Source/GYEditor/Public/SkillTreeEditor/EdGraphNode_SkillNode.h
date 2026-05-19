// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphNode_SkillNode.generated.h"

class USkillNodeDataAsset;
/**
 *
 */
UCLASS()
class GYEDITOR_API UEdGraphNode_SkillNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<USkillNodeDataAsset> SkillAsset;

	static const FName PinCategory;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool CanDuplicateNode() const override { return true; }
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual void PostPasteNode() override;

	UEdGraphPin* GetInputPin()  const;
	UEdGraphPin* GetOutputPin() const;
};
