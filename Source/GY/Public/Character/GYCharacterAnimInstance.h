// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GYCharacterAnimInstance.generated.h"

class UCharacterMovementComponent;
class ACharacter;
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

protected:

	UPROPERTY()
	ACharacter* OwnerCharacter;
	UPROPERTY()
	UCharacterMovementComponent* MovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bHasAcceleration;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling;

	//이동 애니매이션을 위한 최소 속도 임계값
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Thresholds")
	float MinSpeedThreshold = 3.0f;
};
