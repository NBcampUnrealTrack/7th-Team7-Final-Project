#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_AimAtTarget.generated.h"

/**
 * 컨트롤러 회전 태스크 / 특정 대상 바라보게 만들 떄 씀
 */
UCLASS()
class GY_API UAbilityTask_AimAtTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ability|Task",
		meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true"))
	static UAbilityTask_AimAtTarget* Create(
		UGameplayAbility* OwningAbility,
		FName TargetBlackboardKey);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	FName BlackboardKey = NAME_None;
};
