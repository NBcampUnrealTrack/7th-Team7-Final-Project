// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeEditor/SkillTreeGraphSchema.h"


#include "SkillTreeEditor/EdGraphNode_SkillNode.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "Framework/Commands/GenericCommands.h"
#include "ToolMenu.h"
#include "SkillTreeEditor/SkillTreeGraphSchemaAction.h"

const FPinConnectionResponse USkillTreeGraphSchema::CanCreateConnection(
	const UEdGraphPin* A, const UEdGraphPin* B) const
{
	ensureMsgf((A)&&(B), TEXT("Pin is nullptr"));

	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(
			CONNECT_RESPONSE_DISALLOW,
			NSLOCTEXT("SkillTree", "SameNode", "같은 노드"));
	}
	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(
			CONNECT_RESPONSE_DISALLOW,
			NSLOCTEXT("SkillTree", "SameDir", "방향 불일치"));
	}
	const UEdGraphPin* InputPin = (A->Direction == EGPD_Input) ? A : B;
	if (InputPin->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(
			CONNECT_RESPONSE_BREAK_OTHERS_A,
			NSLOCTEXT("SkillTree", "Replace", "기존 부모 교체"));
	}
	return FPinConnectionResponse(
		CONNECT_RESPONSE_MAKE,
		FText::GetEmpty());
}

bool USkillTreeGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	ensureMsgf((A)&&(B), TEXT("Pin is nullptr"));

	const bool bModified = UEdGraphSchema::TryCreateConnection(A, B);
	if (bModified)
	{
		A->GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
	return bModified;
}

void USkillTreeGraphSchema::GetContextMenuActions(
	UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (false == IsValid(Menu) || false == IsValid(Context)) return;

	if (!Context->Node) return;

	FToolMenuSection& Section = Menu->AddSection("SkillNodeActions",
	                                             NSLOCTEXT("SkillTree", "NodeActions", "노드"));

	Section.AddMenuEntry(FGenericCommands::Get().Delete);
	Section.AddMenuEntry(FGenericCommands::Get().Cut);
	Section.AddMenuEntry(FGenericCommands::Get().Copy);
	Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
}

FLinearColor USkillTreeGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	if (PinType.PinCategory == UEdGraphNode_SkillNode::PinCategory)
	{
		return FLinearColor(0.1f, 0.9f, 0.4f);
	}
	return FLinearColor::White;
}

void USkillTreeGraphSchema::DroppedAssetsOnGraph(
	const TArray<FAssetData>& Assets,
	const FVector2D& GraphPosition,
	UEdGraph* Graph) const
{
	FVector2D Offset = FVector2D::ZeroVector;
	for (const FAssetData& Asset : Assets)
	{
		if (USkillNodeDataAsset* SkillAsset = Cast<USkillNodeDataAsset>(Asset.GetAsset()))
		{
			SpawnSkillNode(Graph, SkillAsset, GraphPosition + Offset);
			Offset += FVector2D(130.f, 0.f);
		}
	}
}

void USkillTreeGraphSchema::DroppedAssetsOnNode(
	const TArray<FAssetData>& Assets,
	const FVector2D& GraphPosition,
	UEdGraphNode* Node) const
{
	if (Assets.Num() != 1) return;
	if (UEdGraphNode_SkillNode* SkillNode = Cast<UEdGraphNode_SkillNode>(Node))
	{
		if (USkillNodeDataAsset* Asset = Cast<USkillNodeDataAsset>(Assets[0].GetAsset()))
		{
			SkillNode->SkillAsset = Asset;
			Node->GetGraph()->NotifyGraphChanged();
		}
	}
}

bool USkillTreeGraphSchema::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
	return CurrentCacheID != InVisualizationCacheID;
}

int32 USkillTreeGraphSchema::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheID;
}

void USkillTreeGraphSchema::ForceVisualizationCacheClear() const
{
	++CurrentCacheID;
}

void USkillTreeGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	TSharedPtr<FSkillTreeGraphSchemaAction_NewNode> Action =
		MakeShared<FSkillTreeGraphSchemaAction_NewNode>(
			NSLOCTEXT("SkillTree", "SkillNodeCategory", "스킬 노드"),
			NSLOCTEXT("SkillTree", "AddSkillNode", "새 스킬 노드 생성"),
			NSLOCTEXT("SkillTree", "AddSkillNodeTooltip", "새 USkillNodeDataAsset을 생성하고 노드로 추가합니다."),
			0);

	ContextMenuBuilder.AddAction(Action);
}

UEdGraphNode_SkillNode* USkillTreeGraphSchema::SpawnSkillNode(
	UEdGraph* Graph, USkillNodeDataAsset* Asset, const FVector2D& Position) const
{
	if (false == IsValid(Graph)) return nullptr;

	UEdGraphNode_SkillNode* NewNode = NewObject<UEdGraphNode_SkillNode>(Graph, NAME_None, RF_Transactional);
	NewNode->SkillAsset = Asset;
	NewNode->NodePosX = Position.X;
	NewNode->NodePosY = Position.Y;
	NewNode->CreateNewGuid();
	NewNode->PostPlacedNewNode();
	Graph->AddNode(NewNode, true, true);
	NewNode->AllocateDefaultPins();
	Graph->NotifyGraphChanged();
	return NewNode;
}
