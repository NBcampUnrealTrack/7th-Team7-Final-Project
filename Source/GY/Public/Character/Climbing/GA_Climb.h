#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GA_Climb.generated.h"

class UAbilityTask_ApplyRootMotionMoveToForce;
class UAbilityTask_PlayMontageAndWait;
class UGYCharacterMovementComponent;
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
	TWeakObjectPtr<UGYCharacterMovementComponent> CachedMovement;

	bool bEnteredFromTop = false;

	// 몽타주 OnBlendOut+OnCompleted 이중 바인딩으로 완료 핸들러가 두 번 도는 것을 막는 가드
	// (2회차는 StartClimbing 중복 호출 / 루트모션 태스크 중복 생성으로 이동 버그 유발)
	bool bEntryMontageHandled = false;
	bool bExitMontageHandled = false;

	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
	bool bSavedUseControllerRotationYaw = false;
	bool bSavedUseControllerDesiredRotation = true;

	UPROPERTY(EditDefaultsOnly, Category="Climb")
	float TopEntryThreshold = 50.f;

	//TODO avater에서 가져오는게 이상적임
	UPROPERTY(EditDefaultsOnly, Category="Climb|Anim")
	TObjectPtr<UAnimMontage> EntryFromTopMontage;

	UPROPERTY(EditDefaultsOnly, Category="Climb|Anim")
	TObjectPtr<UAnimMontage> ExitToTopMontage;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;
	UPROPERTY(EditDefaultsOnly, Category="Climb|Entry")
	float EntryInterpDuration = 0.3f;

	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> EntryMoveTask;

	FTransform PendingEntryTransform;
#pragma region ExitNavSnap
	UFUNCTION()
	void OnExitNavSnapFinished();

	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> ExitNavSnapTask;

	UPROPERTY(EditDefaultsOnly, Category="Climb|Exit")
	float ExitNavSnapDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category="Climb|Exit")
	FVector NavProjectExtent = FVector(200.f, 200.f, 300.f);

#pragma endregion
	UFUNCTION()
	void OnEntryMoveFinished();

	UFUNCTION() void OnEntryMontageCompleted();
	UFUNCTION() void OnEntryMontageInterrupted();
	UFUNCTION() void OnExitMontageCompleted();
	UFUNCTION() void OnExitMontageInterrupted();
};
