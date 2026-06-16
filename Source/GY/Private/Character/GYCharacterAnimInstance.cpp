// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYCharacterAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Core/GameplayTags/StateTags.h"


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

	if (AbilitySystemComponent == nullptr && OwnerCharacter != nullptr)
	{
		AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);
	}

	if (AbilitySystemComponent != nullptr)
	{
		bIsStunned = AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Hit_Stun);
		bIsStaggered = AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Hit_Stagger);
	}

	if (IsValid(AbilitySystemComponent) && IsValid(OwnerCharacter) && AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Climbing))
	{
		bIsClimbing = true;
		const FVector CurrentVelocity = OwnerCharacter->GetVelocity();
		const FVector ClimbAxis = OwnerCharacter->GetActorUpVector();
		const float VertSpeed = FVector::DotProduct(CurrentVelocity, ClimbAxis);
		float MaxSpeed = 150.f;
		UGYCharacterMovementComponent* GyCharacterMovementComponent = Cast<UGYCharacterMovementComponent> (OwnerCharacter->GetMovementComponent());
		if (GyCharacterMovementComponent != nullptr)
		{
			MaxSpeed = GyCharacterMovementComponent->GetMaxClimbSpeed();
		}
		ClimbPlayRate = FMath::Clamp(VertSpeed / MaxSpeed, -1.f, 1.f);
	}
	else
	{
		bIsClimbing = false;
		ClimbPlayRate = 0.f;
	}

}

void UGYCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

}
