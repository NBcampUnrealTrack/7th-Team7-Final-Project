// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GYSkillNodeWidget.h"
#include "Widget/Interact/GYTimeRiftPanelWidget.h"
#include "GYSkillTreeWidget.generated.h"

class UGYAbilitySystemComponent;
class UGYSkillNodeWidget;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;
class USkillNodeDataAsset;
class USkillTreeDataAsset;
class UCanvasPanel;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UWidget;
class UButton;
class UGYSkillConnectionLinesWidget;
/**
 *
 */
UCLASS()
class GYUI_API UGYSkillTreeWidget : public UGYTimeRiftPanelWidget
{
	GENERATED_BODY()

public:

protected:
	virtual FGameplayTag GetExitEventTag() const override;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
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

	ESkillNodeType ResolveNodeType(USkillNodeDataAsset* Node) const;

	UFUNCTION()
	void HandleNodeClicked(USkillNodeDataAsset* Node);

	UFUNCTION()
	void HandleNodeHovered(UGYSkillNodeWidget* NodeWidget);

	UFUNCTION()
	void HandleNodeUnhovered(UGYSkillNodeWidget* NodeWidget);

	void UpdateSkillPointText();

	/** 정보 팝업 표시/숨김/추적 */
	void ShowTooltipFor(UGYSkillNodeWidget* NodeWidget);
	void HideTooltip();
	void UpdateTooltipPosition();

	/** 연결선 위젯 호출*/
	void PaintConnections(const FGeometry& LinesGeo, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
	/** 노드 위치 바운딩 박스 */
	bool GetNodeBounds(FVector2D& OutMin, FVector2D& OutMax) const;
	/** 노드 전체를 담도록 SizeBox 크기 갱신 */
	void UpdateContentSize();
	/** 우클릭 드래그로 계산한 오프셋을 스크롤 박스에 반영 */
	void ApplyScrollOffsets();
	/** 전체 노드가 화면 중앙에 오도록 스크롤 위치 지정 */
	void CenterOnNodes(const FVector2D& ViewSize);

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;


	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResetButton;

	UFUNCTION()
	void OnCloseButtonClicked();

	UFUNCTION()
	void OnResetButtonClicked();

	UGYAbilitySystemComponent* GetOwnerASC() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SkillTreeScrollV;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SkillTreeCanvasSizeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SkillTreeScrollH;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> SkillTreeCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UGYSkillConnectionLinesWidget> ConnectionLines;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillPointText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SkillTooltip;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TooltipNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TooltipDescText;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree")
	TSubclassOf<UGYSkillNodeWidget> SkillNodeWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|View")
	float PanSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|View")
	bool bCenterOnOpen = true;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|View")
	float ContentPadding = 200.f;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Node")
	ESkillNodeType DefaultNodeType = ESkillNodeType::PrimaryStat;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Node")
	TMap<TObjectPtr<USkillNodeDataAsset>, ESkillNodeType> NodeTypeOverrides;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Connection")
	FLinearColor ConnectionColorLocked = FLinearColor(1.f, 1.f, 1.f, 0.08f);

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Connection")
	FLinearColor ConnectionColorUnlocked = FLinearColor(0.30f, 1.0f, 0.50f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Connection")
	FLinearColor ConnectionColorAvailable = FLinearColor(1.0f, 0.90f, 0.25f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Connection")
	float ConnectionThickness = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Connection")
	float ConnectionThicknessHighlighted = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Connection")
	float ConnectionEdgeGap = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Tooltip")
	FText SkillPointTextFormat;

	UPROPERTY(EditDefaultsOnly, Category="SkillTree|Tooltip")
	FVector2D TooltipMouseOffset = FVector2D(16.f, 16.f);

	UPROPERTY()
	TObjectPtr<USkillTreeDataAsset> SkillTreeData;

	UPROPERTY()
	TMap<TObjectPtr<USkillNodeDataAsset>, TObjectPtr<UGYSkillNodeWidget>> NodeWidgets;

private:
	FDelegateHandle SkillPointHandle;

	bool bIsPanning = false;
	FVector2D LastMouseScreenPos = FVector2D::ZeroVector;
	FVector2D PanTarget = FVector2D::ZeroVector;
	bool bPendingCenter = false;

	UPROPERTY()
	TObjectPtr<UGYSkillNodeWidget> HoveredNodeWidget;
};
