#include "SkillTreeEditor/SkillTreeGraphSchemaAction.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/DataAssetFactory.h"
#include "SkillTree/SkillNodeDataAsset.h"
#include "SkillTree/SkillTreeDataAsset.h"
#include "SkillTreeEditor/EdGraphNode_SkillNode.h"

UEdGraphNode* FSkillTreeGraphSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
                                                                 const FVector2D Location, bool bSelectNewNode)
{
	return CreateNewSkillNode(ParentGraph, FromPin, Location, bSelectNewNode);
}

UEdGraphNode* FSkillTreeGraphSchemaAction_NewNode::CreateNewSkillNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
	const FVector2D& Location, bool bSelectNewNode)
{
	if (false == IsValid(ParentGraph)) return nullptr;

	FString PackagePath = TEXT("/Game/SkillNodes");
	if (USkillTreeDataAsset* TreeAsset =
	Cast<USkillTreeDataAsset>(ParentGraph->GetOuter()))
	{
		PackagePath = FPackageName::GetLongPackagePath(TreeAsset->GetOutermost()->GetName());
	}
	FString AssetName = TEXT("MySkillNode");

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.CreateUniqueAssetName(PackagePath / AssetName, TEXT(""), PackagePath, AssetName);

	UDataAssetFactory* DataAssetFactory = NewObject<UDataAssetFactory>();
	DataAssetFactory->DataAssetClass = USkillNodeDataAsset::StaticClass();

	FString FinalPackagePath = FPackageName::GetLongPackagePath(PackagePath);

	USkillNodeDataAsset* NewSkillNodeDataAsset = Cast<USkillNodeDataAsset>(AssetTools.CreateAsset(AssetName, FinalPackagePath, DataAssetFactory->DataAssetClass, DataAssetFactory));

	if (false == IsValid(NewSkillNodeDataAsset)) return nullptr;

	UEdGraphNode_SkillNode* NewNode = NewObject<UEdGraphNode_SkillNode>(ParentGraph, NAME_None, RF_Transactional);
	NewNode->SkillAsset = NewSkillNodeDataAsset;
	NewNode->NodePosX = Location.X;
	NewNode->NodePosY = Location.Y;
	NewNode->CreateNewGuid();
	NewNode->PostPlacedNewNode();
	ParentGraph->AddNode(NewNode,true, bSelectNewNode);
	NewNode->AllocateDefaultPins();

	if (FromPin)
	{
		if (FromPin->Direction == EGPD_Output)
		{
			UEdGraphPin* InputPin = NewNode->GetInputPin();
			if (InputPin)
			{
				FromPin->MakeLinkTo(InputPin);
			}
		}
		else
		{
			UEdGraphPin* OutputPin = NewNode->GetOutputPin();
			if (OutputPin)
			{
				OutputPin->MakeLinkTo(FromPin);
			}
		}
	}
	ParentGraph->NotifyGraphChanged();
	return NewNode;

}
