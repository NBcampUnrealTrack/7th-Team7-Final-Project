// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_Climb.generated.h"

class UAbilityTask_LadderClimb;
class ALadder;
/**
 *
 */
UCLASS()
class GY_API UGA_Climb : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Climb(const FObjectInitializer& ObjectInitializer);

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;


protected:
	UFUNCTION()
	void OnClimbExit(ELadderExitReason Reason);

	bool ShouldEnterFromTop(const ACharacter* Character, const ALadder* Ladder) const;

	UPROPERTY()
	TWeakObjectPtr<ALadder> CurrentLadder;

	UPROPERTY()
	TObjectPtr<UAbilityTask_LadderClimb> ClimbTask;

	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
	bool bSavedOrientToMovement = true;
	bool bSavedUseControllerRotationYaw = false;

	UPROPERTY(EditDefaultsOnly, Category="Climb")
	float ClimbSpeed = 150.f;
	UPROPERTY(EditDefaultsOnly, Category="Climb")
	float TopEntryThreshold = 50.f;
	UPROPERTY(EditDefaultsOnly, Category="Climb")
	float TopExitForwardOffset = 60.f;

};
