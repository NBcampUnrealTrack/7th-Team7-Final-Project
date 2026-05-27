// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYCharacterAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


void UGYCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (OwnerCharacter != nullptr)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();

		RunningSpeed = MovementComponent->MaxWalkSpeed;
	}

}

void UGYCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (OwnerCharacter && MovementComponent)
	{

		Velocity = MovementComponent->Velocity;
		GroundSpeed = Velocity.Size2D();




		bHasAcceleration = !MovementComponent->GetCurrentAcceleration().IsNearlyZero();

		bShouldMove = (GroundSpeed > MinSpeedThreshold) && bHasAcceleration;

		if (GroundSpeed > MinSpeedThreshold)
		{
			Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, OwnerCharacter->GetActorRotation());
		}

		if (bHasAcceleration)
		{
			bIsRunning = FMath::IsNearlyEqual(GroundSpeed, RunningSpeed);
		}

		bIsFalling = MovementComponent->IsFalling();



	}
}

void UGYCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

}
