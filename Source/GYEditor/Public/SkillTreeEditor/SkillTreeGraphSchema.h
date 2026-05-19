// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "SkillTreeGraphSchema.generated.h"

/**
 *
 */
UCLASS()
class GYEDITOR_API USkillTreeGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	virtual const FPinConnectionResponse CanCreateConnection(
		const UEdGraphPin* A, const UEdGraphPin* B) const override;

	virtual bool TryCreateConnection(
		UEdGraphPin* A, UEdGraphPin* B) const override;

	virtual void GetContextMenuActions(
		UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

	virtual FLinearColor GetPinTypeColor(
		const FEdGraphPinType& PinType) const override;

	virtual void DroppedAssetsOnGraph(
		const TArray<FAssetData>& Assets,
		const FVector2D& GraphPosition,
		UEdGraph* Graph) const override;

	virtual void DroppedAssetsOnNode(
		const TArray<FAssetData>& Assets,
		const FVector2D& GraphPosition,
		UEdGraphNode* Node) const override;

	virtual bool IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const override;
	virtual int32 GetCurrentVisualizationCacheID() const override;
	virtual void ForceVisualizationCacheClear() const override;

private:
	class UEdGraphNode_SkillNode* SpawnSkillNode(
		UEdGraph* Graph,
		class USkillNodeDataAsset* Asset,
		const FVector2D& Position) const;

	mutable int32 CurrentCacheID = 0;
};
