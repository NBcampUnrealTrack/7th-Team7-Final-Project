// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYCharacterAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Core/GameplayTags/StateTags.h"
#include "Logging/GYLogManager.h"


void UGYCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (OwnerCharacter != nullptr)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
		AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);

		RunningSpeed = 600.f;
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

		BrakingDeceleration = MovementComponent->BrakingDecelerationWalking;
		BrakingFriction = MovementComponent->BrakingFriction;

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

	//스턴, 스태거 로직
	if (AbilitySystemComponent == nullptr && OwnerCharacter != nullptr)
	{
		AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);
	}

	if (AbilitySystemComponent != nullptr)
	{
		bIsStunned = AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Hit_Stun);
		bIsStaggered = AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Hit_Stagger);
	}


	//사다리 로직
	UGYCharacterMovementComponent* GyMovement = Cast<UGYCharacterMovementComponent>(MovementComponent);
	const bool bIsClimbingMode = GyMovement && GyMovement->IsClimbing();

	bIsClimbing = bIsClimbingMode;

	if (bIsClimbingMode  && IsValid(OwnerCharacter))
	{
		const FVector CurrentVelocity = OwnerCharacter->GetVelocity();
		const FVector ClimbAxis = OwnerCharacter->GetActorUpVector();
		const float VertSpeed = FVector::DotProduct(CurrentVelocity, ClimbAxis);
		float MaxSpeed = 150.f;
		if (GyMovement != nullptr)
		{
			MaxSpeed = GyMovement->GetMaxClimbSpeed();
		}
		ClimbPlayRate = FMath::Clamp(VertSpeed / MaxSpeed, -1.f, 1.f);
	}
	else
	{
		ClimbPlayRate = 0.f;
	}

}

void UGYCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	CalculateDistanceToMatch(DeltaSeconds);
}

void UGYCharacterAnimInstance::CalculateDistanceToMatch(float DeltaSeconds)
{

	if (!bHasAcceleration && GroundSpeed > 0.f)
	{
		//가속도 (-방향) : 브레이크가속도 + 마찰력(v*friction)
		const float EffectiveBraking = BrakingDeceleration + (BrakingFriction * GroundSpeed);
		if (EffectiveBraking > 0.f)
		{
			// 등가속도시 이동거리  = v^2/2a
			//

			DistanceToMatch =  (GroundSpeed * GroundSpeed) / (2.f * EffectiveBraking);


			return;
		}
	}
	DistanceToMatch = 0.f;
}
