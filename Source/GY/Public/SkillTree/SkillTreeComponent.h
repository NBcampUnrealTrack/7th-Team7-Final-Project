#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillTreeComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnSkillTreeChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API USkillTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillTreeComponent();

	UFUNCTION(BlueprintPure, Category="SkillTree")
	bool IsNodeUnlocked(const USkillNodeDataAsset* Node) const;

	UFUNCTION(BlueprintPure, Category="SkillTree")
	bool HasAllPrerequisites(const USkillNodeDataAsset* Node) const;

	const TArray<FPrimaryAssetId>& GetUnlockedNodes() const { return UnlockedNodes; }

	bool UnlockNode(const USkillNodeDataAsset* Node);

	FOnSkillTreeChanged OnSkillTreeChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_UnlockedNodes();

	UPROPERTY(ReplicatedUsing=OnRep_UnlockedNodes)
	TArray<FPrimaryAssetId> UnlockedNodes;
};
