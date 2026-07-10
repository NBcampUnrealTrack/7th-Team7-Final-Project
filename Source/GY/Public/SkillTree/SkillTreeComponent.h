#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Persistence/GYSaveable.h"
#include "Persistence/GYSaveSectionKeys.h"
#include "SkillTreeComponent.generated.h"

class UAbilitySystemComponent;
class USkillNodeDataAsset;

DECLARE_MULTICAST_DELEGATE(FOnSkillTreeChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API USkillTreeComponent : public UActorComponent, public IGYSaveable
{
	GENERATED_BODY()

public:
	USkillTreeComponent();

	UFUNCTION(BlueprintPure, Category="SkillTree")
	bool IsNodeUnlocked(const USkillNodeDataAsset* Node) const;

	UFUNCTION(BlueprintPure, Category="SkillTree")
	bool HasAllPrerequisites(const USkillNodeDataAsset* Node) const;

	const TArray<FPrimaryAssetId>& GetUnlockedNodes() const { return UnlockedNodes; }

	// [SERVER] 노드 언락 + SkillEffect 적용. SP 비용/선행조건 검증은 호출자(GA_SkillUnlock) 소관
	bool UnlockNode(const USkillNodeDataAsset* Node);

	UFUNCTION(Server, Reliable)
	void ServerResetSkillTree();

	UFUNCTION(BlueprintCallable, Category="SkillTree")
	void ResetSkillTree();


	FOnSkillTreeChanged OnSkillTreeChanged;

	// IGYSaveable — SP 잔량 차감은 스탯 복원(총량 리셋) 이후여야 함
	virtual FString GetSaveSectionKey() const override { return GYSaveSectionKeys::SkillTree; }
	virtual TSharedPtr<FJsonValue> ExportSaveData() const override;
	virtual void ImportSaveData(const TSharedPtr<FJsonValue>& Data) override;
	virtual TArray<FString> GetRestoreDependencies() const override { return { GYSaveSectionKeys::Stats }; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_UnlockedNodes();

	UPROPERTY(ReplicatedUsing=OnRep_UnlockedNodes)
	TArray<FPrimaryAssetId> UnlockedNodes;

private:
	UAbilitySystemComponent* ResolveASC() const;
	const USkillNodeDataAsset* FindNodeById(const FPrimaryAssetId& NodeId) const;
	void ApplySkillEffect(const USkillNodeDataAsset* Node);

	// 이 컴포넌트가 적용한 노드 효과 핸들 — 복원 재적용 시 이전 것 제거 (중복 적용 방지)
	TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;
};
