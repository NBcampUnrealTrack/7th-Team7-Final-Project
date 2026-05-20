#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "SkillTreeGraphSchemaAction.generated.h"

USTRUCT()
struct FSkillTreeGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY()

	FSkillTreeGraphSchemaAction_NewNode() : FEdGraphSchemaAction() {}
	FSkillTreeGraphSchemaAction_NewNode(
		FText InNodeCategory,
		FText InMenuDesc,
		FText InToolTip,
		int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
	{}

	// 핀을 놓았을 때 실제로 실행되는 함수
	virtual UEdGraphNode* PerformAction(
		UEdGraph* ParentGraph,
		UEdGraphPin* FromPin,
		const FVector2D Location,
		bool bSelectNewNode = true) override;

private:
	// 에셋 생성 + 노드 스폰
	UEdGraphNode* CreateNewSkillNode(
		UEdGraph* ParentGraph,
		UEdGraphPin* FromPin,
		const FVector2D& Location,
		bool bSelectNewNode);
};
