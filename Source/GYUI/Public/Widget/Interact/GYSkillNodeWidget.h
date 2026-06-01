// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/GYUserWidget.h"
#include "GYSkillNodeWidget.generated.h"

class UButton;
class USkillNodeDataAsset;
/**
 *
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillNodeClicked, USkillNodeDataAsset*, Node);

UENUM(BlueprintType)
enum class ESkillNodeState : uint8
{
	Locked,
	Unlockable,
	Unlocked,
};


UCLASS()
class GYUI_API UGYSkillNodeWidget : public UGYUserWidget
{
	GENERATED_BODY()


public:

	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable)
	FOnSkillNodeClicked OnNodeClicked;

	UFUNCTION(BlueprintCallable, Category="SkillNode")
	void SetNodeData(USkillNodeDataAsset* InNodeData);

	UFUNCTION(BlueprintCallable, Category="SkillNode")
	void SetNodeState(ESkillNodeState InState);

	UFUNCTION(BlueprintPure, Category="SkillNode")
	USkillNodeDataAsset* GetNodeData() const { return NodeData; }

	UFUNCTION(BlueprintPure, Category="SkillNode")
	ESkillNodeState GetNodeState() const { return NodeState; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="SkillNode")
	void OnNodeDataChanged(USkillNodeDataAsset* Node);

	UFUNCTION(BlueprintImplementableEvent, Category="SkillNode")
	void OnNodeStateChanged(ESkillNodeState NewState);

	UFUNCTION(BlueprintCallable, Category="SkillNode")
	void HandleNodeClicked();

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkillNodeDataAsset> NodeData;

	UPROPERTY(BlueprintReadOnly)
	ESkillNodeState NodeState = ESkillNodeState::Locked;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NodeButton;
};
