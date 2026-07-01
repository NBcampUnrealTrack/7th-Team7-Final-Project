#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_PlayMontageOnMesh.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGYMontageOnMeshDelegate);

UCLASS()
class GY_API UAbilityTask_PlayMontageOnMesh : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_PlayMontageOnMesh(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility",
				BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_PlayMontageOnMesh* PlayMontageOnMesh(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		USkeletalMeshComponent* TargetMesh,
		UAnimMontage* Montage,
		float Rate = 1.f,
		FName StartSection = NAME_None,
		bool bStopWhenAbilityEnds = true);

	UPROPERTY(BlueprintAssignable)
	FGYMontageOnMeshDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FGYMontageOnMeshDelegate OnInterrupted;

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* InMontage, bool bInterrupted);

private:
	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	float PlayRate = 1.f;
	FName SectionName = NAME_None;
	bool bStopOnEnd = true;

	FOnMontageBlendingOutStarted BlendingOutDelegate;
};
