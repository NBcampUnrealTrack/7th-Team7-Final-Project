// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/GYAnimTypes.h"
#include "GYCharacterAnimInstance.generated.h"

class UCharacterMovementComponent;
class ACharacter;
class UAbilitySystemComponent;
/**
 *
 */


UCLASS()
class GY_API UGYCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	float GetDirection() const { return Direction; };
	bool GetIsRunning() const { return bIsRunning; };

protected:
	UFUNCTION(meta=(BlueprintThreadSafe))
	void CalculateDistanceToMatch(float DeltaSeconds);


	UPROPERTY()
	ACharacter* OwnerCharacter;
	UPROPERTY()
	UCharacterMovementComponent* MovementComponent;

	UPROPERTY()
	UAbilitySystemComponent* AbilitySystemComponent;

	// 행동불능(CC) 상태 — State.Hit.Stun / State.Hit.Stagger 태그를 반영. State Machine 전이에 사용.
	UPROPERTY(BlueprintReadOnly, Category = "CC", meta = (BlueprintThreadSafe))
	bool bIsStunned;

	UPROPERTY(BlueprintReadOnly, Category = "CC", meta = (BlueprintThreadSafe))
	bool bIsStaggered;

	UPROPERTY(BlueprintReadOnly, Category="Climb", meta=(BlueprintThreadSafe))
	bool bIsClimbing = false;

	UPROPERTY(BlueprintReadOnly, Category="Climb", meta=(BlueprintThreadSafe))
	float ClimbPlayRate = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	float LastDirection;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	bool bShouldMove;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	bool bHasAcceleration;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	bool bIsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	bool bIsRunning;

	//디스턴스 매칭을 위한 정지거리
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion", meta = (BlueprintThreadSafe))
	float DistanceToMatch = 0.f;

	//이동 애니매이션을 위한 최소 속도 임계값
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Thresholds")
	float MinSpeedThreshold = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Thresholds")
	float RunningSpeed = 600.f;

	float BrakingDeceleration = 0.f;
	float BrakingFriction = 0.f;

	// 알맞은 스탑 모션 재생
	UPROPERTY(EditDefaultsOnly, Category = "DistanceMatching", meta=(BlueprintThreadSafe))
	FGYDirectionalStopAnims WalkStopAnimSet;

	UPROPERTY(EditDefaultsOnly, Category = "DistanceMatching", meta=(BlueprintThreadSafe))
	FGYDirectionalStopAnims RunStopAnimSet;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "DistanceMatching|Output", meta=(BlueprintThreadSafe))
	UAnimSequence* TargetStopSequence = nullptr;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* MoveBlendSpace;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequence* IdleSequence;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Climb", meta=(BlueprintThreadSafe))
	UAnimSequence* ClimbCycleSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* WalkStartBlendSpace;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* RunStartBlendSpace;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* WalkStopBlendSpace;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* RunStopBlendSpace;
};
