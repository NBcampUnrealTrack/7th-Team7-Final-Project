// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GYSkillNodeWidget.h"
#include "Core/GYActivatableWidget.h"
#include "GYSkillTreeWidget.generated.h"

class UGYAbilitySystemComponent;
class UGYSkillNodeWidget;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;
class USkillNodeDataAsset;
class USkillTreeDataAsset;
class UCanvasPanel;
/**
 *
 */
UCLASS()
class GYUI_API UGYSkillTreeWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;


	void RebuildTree();
	void CollectAllNodes(USkillNodeDataAsset* RootNode, TSet<USkillNodeDataAsset*>& Collected);
	void CreateNodeWidget(USkillNodeDataAsset* Node);

	void Refresh();
	ESkillNodeState ComputeNodeState(USkillNodeDataAsset* Node) const;
	bool IsNodeUnlocked(USkillNodeDataAsset* Node) const;

	UFUNCTION()
	void HandleNodeClicked(USkillNodeDataAsset* Node);

	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	void ApplyCanvasTransform();

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|View")
	float ZoomStep = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|View")
	float MinZoom = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|View")
	float MaxZoom = 3.0f;


	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> CloseButton;

	UFUNCTION()
	void OnCloseButtonClicked();

	UGYAbilitySystemComponent* GetOwnerASC() const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> SkillTreeCanvas;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree")
	TSubclassOf<UGYSkillNodeWidget> SkillNodeWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree")
	FLinearColor ConnectionColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree")
	float ConnectionThickness = 2.0f;

	UPROPERTY()
	TObjectPtr<USkillTreeDataAsset> SkillTreeData;

	UPROPERTY()
	TMap<TObjectPtr<USkillNodeDataAsset>, TObjectPtr<UGYSkillNodeWidget>> NodeWidgets;

private:
	FDelegateHandle SkillPointHandle;
	float CanvasZoom = 1.0f;
	FVector2D CanvasPan = FVector2D::ZeroVector;
	bool bIsPanning = false;
	FVector2D LastMouseScreenPos = FVector2D::ZeroVector;
};
