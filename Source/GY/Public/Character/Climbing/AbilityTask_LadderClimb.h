#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_LadderClimb.generated.h"

class ALadder;

UENUM(BlueprintType)
enum class ELadderExitReason : uint8
{
	Top,
	Bottom,
	LadderInvalid,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLadderClimbExitDelegate, ELadderExitReason, Reason);


/**
 *
 */
UCLASS()
class GY_API UAbilityTask_LadderClimb : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_LadderClimb(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category="Ability|Tasks|Climb",
		meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility",
			  BlueprintInternalUseOnly="true"))
	static UAbilityTask_LadderClimb* CreateLadderClimbTask(
		UGameplayAbility* OwningAbility,
		ALadder* Ladder,
		float ClimbSpeed = 150.f);

	UPROPERTY(BlueprintAssignable)
	FOnLadderClimbExitDelegate OnExit;

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

protected:
	UPROPERTY()
	TWeakObjectPtr<ALadder> Ladder;

	float ClimbSpeed = 150.f;

	void BroadcastExit(ELadderExitReason Reason);
};
