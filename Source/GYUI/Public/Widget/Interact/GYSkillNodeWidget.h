#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYSkillNodeWidget.generated.h"

class UButton;
class UImage;
class USkillNodeDataAsset;
class UGYSkillNodeWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillNodeClicked, USkillNodeDataAsset*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillNodeHoverChanged, UGYSkillNodeWidget*, NodeWidget);

UENUM(BlueprintType)
enum class ESkillNodeState : uint8
{
	Locked,
	Unlockable,
	Unlocked,
};

/** 노드 성격 구분 — STR/DEX 같은 1차 수치 노드 vs 액션 메커니즘 활성화 노드 */
UENUM(BlueprintType)
enum class ESkillNodeType : uint8
{
	PrimaryStat		UMETA(DisplayName = "Primary Stat (STR/DEX...)"),
	ActionMechanic	UMETA(DisplayName = "Action Mechanic"),
};


UCLASS()
class GYUI_API UGYSkillNodeWidget : public UGYUserWidget
{
	GENERATED_BODY()


public:

	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable)
	FOnSkillNodeClicked OnNodeClicked;

	UPROPERTY(BlueprintAssignable)
	FOnSkillNodeHoverChanged OnNodeHovered;

	UPROPERTY(BlueprintAssignable)
	FOnSkillNodeHoverChanged OnNodeUnhovered;

	UFUNCTION(BlueprintCallable, Category="SkillNode")
	void SetNodeData(USkillNodeDataAsset* InNodeData);

	UFUNCTION(BlueprintCallable, Category="SkillNode")
	void SetNodeState(ESkillNodeState InState);

	UFUNCTION(BlueprintCallable, Category="SkillNode")
	void SetNodeType(ESkillNodeType InType);

	UFUNCTION(BlueprintPure, Category="SkillNode")
	USkillNodeDataAsset* GetNodeData() const { return NodeData; }

	UFUNCTION(BlueprintPure, Category="SkillNode")
	ESkillNodeState GetNodeState() const { return NodeState; }

	UFUNCTION(BlueprintPure, Category="SkillNode")
	ESkillNodeType GetNodeType() const { return NodeType; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="SkillNode")
	void OnNodeDataChanged(USkillNodeDataAsset* Node);

	UFUNCTION(BlueprintImplementableEvent, Category="SkillNode")
	void OnNodeStateChanged(ESkillNodeState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="SkillNode")
	void OnNodeTypeChanged(ESkillNodeType NewType);

	UFUNCTION(BlueprintCallable, Category="SkillNode")
	void HandleNodeClicked();

	UFUNCTION()
	void HandleNodeHovered();

	UFUNCTION()
	void HandleNodeUnhovered();

	/** 상태/타입/데이터를 실제 비주얼에 반영 */
	void ApplyVisuals();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NodeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> NodeBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectionBorder;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkillNodeDataAsset> NodeData;

	UPROPERTY(BlueprintReadOnly)
	ESkillNodeState NodeState = ESkillNodeState::Locked;

	UPROPERTY(BlueprintReadOnly)
	ESkillNodeType NodeType = ESkillNodeType::PrimaryStat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SkillNode|Style", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LockedOpacity = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SkillNode|Style", meta=(ClampMin="0.0", ClampMax="1.0"))
	float UnlockableOpacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SkillNode|Style", meta=(ClampMin="0.0", ClampMax="1.0"))
	float UnlockedOpacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SkillNode|Style")
	FLinearColor PrimaryStatColor = FLinearColor(0.20f, 0.55f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SkillNode|Style")
	FLinearColor ActionMechanicColor = FLinearColor(1.0f, 0.55f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SkillNode|Style")
	FLinearColor LockedTint = FLinearColor(0.16f, 0.16f, 0.18f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SkillNode|Style")
	FLinearColor UnlockableRingColor = FLinearColor(1.0f, 0.90f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SkillNode|Style")
	FLinearColor UnlockedRingColor = FLinearColor(0.30f, 1.0f, 0.50f, 1.0f);

private:
	FLinearColor ResolveBackgroundColor() const;
};
