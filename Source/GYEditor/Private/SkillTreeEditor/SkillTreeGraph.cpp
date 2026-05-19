// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeEditor/SkillTreeGraph.h"

#include "SkillTree/SkillNodeDataAsset.h"
#include "SkillTree/SkillTreeDataAsset.h"
#include "SkillTreeEditor/EdGraphNode_SkillNode.h"

void USkillTreeGraph::RebuildGraphFromAsset(USkillTreeDataAsset* InAsset)
{
	if (!InAsset) return;

	Nodes.Empty();

	TMap<USkillNodeDataAsset*, UEdGraphNode_SkillNode*> AssetToNodeMap;
	TArray<USkillNodeDataAsset*> AllSkills;
	TQueue<USkillNodeDataAsset*> Queue;

	for (USkillNodeDataAsset* Root : InAsset->RootNodes)
		if (Root) Queue.Enqueue(Root);

	while (!Queue.IsEmpty())
	{
		USkillNodeDataAsset* Current = nullptr;
		Queue.Dequeue(Current);
		if (!Current || AllSkills.Contains(Current)) continue;
		AllSkills.Add(Current);

		for (USkillNodeDataAsset* Child : Current->Children)
			if (Child) Queue.Enqueue(Child);
	}

	for (USkillNodeDataAsset* SkillAsset : AllSkills)
	{
		UEdGraphNode_SkillNode* NewNode =
			NewObject<UEdGraphNode_SkillNode>(this, NAME_None, RF_Transactional);
		NewNode->SkillAsset = SkillAsset;
		NewNode->CreateNewGuid();
		NewNode->PostPlacedNewNode();
		NewNode->AllocateDefaultPins();

		if (const FVector2D* Pos = InAsset->SkillNodePositions.Find(SkillAsset->GetFName()))
		{
			NewNode->NodePosX = Pos->X;
			NewNode->NodePosY = Pos->Y;
		}

		Nodes.Add(NewNode);
		AssetToNodeMap.Add(SkillAsset, NewNode);
	}

	for (auto& Pair : AssetToNodeMap)
	{
		UEdGraphNode_SkillNode* ChildNode = Pair.Value;
		for (USkillNodeDataAsset* ParentAsset : Pair.Key->Prerequisites)
		{
			UEdGraphNode_SkillNode** ParentNode = AssetToNodeMap.Find(ParentAsset);
			if (!ParentNode) continue;
			UEdGraphPin* Out = (*ParentNode)->GetOutputPin();
			UEdGraphPin* In  = ChildNode->GetInputPin();
			if (Out && In) Out->MakeLinkTo(In);
		}
	}

	NotifyGraphChanged();
}

void USkillTreeGraph::CompileAsset(USkillTreeDataAsset* InAsset)
{
	InAsset->RootNodes.Empty();
	InAsset->SkillNodePositions.Empty();

	for (UEdGraphNode* Node : Nodes)
	{
		UEdGraphNode_SkillNode* SkillNode = Cast<UEdGraphNode_SkillNode>(Node);
		if (!SkillNode || !SkillNode->SkillAsset) continue;
		SkillNode->SkillAsset->Prerequisites.Empty();
		SkillNode->SkillAsset->Children.Empty();
	}

	for (UEdGraphNode* Node : Nodes)
	{
		UEdGraphNode_SkillNode* SkillNode = Cast<UEdGraphNode_SkillNode>(Node);
		if (!SkillNode || !SkillNode->SkillAsset) continue;

		InAsset->SkillNodePositions.Add(
			SkillNode->SkillAsset->GetFName(),
			FVector2D(SkillNode->NodePosX, SkillNode->NodePosY));

		UEdGraphPin* InputPin = SkillNode->GetInputPin();
		if (InputPin)
		{
			for (UEdGraphPin* Linked : InputPin->LinkedTo)
			{
				if (UEdGraphNode_SkillNode* Parent =
					Cast<UEdGraphNode_SkillNode>(Linked->GetOwningNode()))
				{
					if (Parent->SkillAsset)
					{
						SkillNode->SkillAsset->Prerequisites.Add(Parent->SkillAsset);
						Parent->SkillAsset->Children.Add(SkillNode->SkillAsset);
					}
				}
			}
		}

		const bool bIsRoot = !InputPin || InputPin->LinkedTo.Num() == 0;
		if (bIsRoot) InAsset->RootNodes.Add(SkillNode->SkillAsset);
	}

	InAsset->MarkPackageDirty();
}
