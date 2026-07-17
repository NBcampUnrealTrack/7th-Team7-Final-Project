#include "Character/GYCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "WorldGimmick/Ladder.h"

UGYCharacterMovementComponent::UGYCharacterMovementComponent(const FObjectInitializer& OI)
    : Super(OI)
{
}

void UGYCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	bDefaultOrientToMovement = bOrientRotationToMovement;
}

void UGYCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
    Super::PhysCustom(DeltaTime, Iterations);
    if (CustomMovementMode == CMOVE_Climbing)
    {
        PhysClimbing(DeltaTime, Iterations);
    }
}

FVector UGYCharacterMovementComponent::GetClimbAxis() const
{
	return ClimbingLadder.IsValid() ? ClimbingLadder->GetClimbAxis() : FVector::UpVector;
}

void UGYCharacterMovementComponent::PushSuppressOrientToMovement()
{
	++OrientToMovementSuppressCount;
    UpdateOrientToMovement();
}

void UGYCharacterMovementComponent::PopSuppressOrientToMovement()
{
	if (OrientToMovementSuppressCount > 0)
	{
		--OrientToMovementSuppressCount;
	}
	UpdateOrientToMovement();
}

void UGYCharacterMovementComponent::UpdateOrientToMovement()
{
	bOrientRotationToMovement = (OrientToMovementSuppressCount == 0) && bDefaultOrientToMovement;
}

void UGYCharacterMovementComponent::PhysClimbing(float DeltaTime, int32 Iterations)
{
	if (!ClimbingLadder.IsValid() || !CharacterOwner)
	{
		EndClimbingWith(ELadderExitReason::LadderInvalid);
		return;
	}
	// 사다리방향 이동벡터
	const FTransform LadderT = ClimbingLadder->GetActorTransform();
	const FVector ClimbAxis = ClimbingLadder->GetClimbAxis();
	const FVector InputVec = Acceleration.GetSafeNormal();

	//수직입력 있는 경우
	float ForwardInput = FVector::DotProduct(InputVec, ClimbAxis);

	//수직입력 없으면 xy기준
	if (FMath::Abs(ForwardInput) < KINDA_SMALL_NUMBER && !InputVec.IsNearlyZero())
	{
		const FVector LocalInput = LadderT.InverseTransformVector(InputVec);
		ForwardInput = -LocalInput.X;
	}

	ForwardInput = FMath::Sign(ForwardInput);

	// 위/아래 이동
	Velocity = ClimbAxis * ForwardInput * MaxClimbSpeed;

	const FQuat TargetQuat = ClimbingLadder->GetClimbFacing().Quaternion();

	FHitResult Hit;
	SafeMoveUpdatedComponent(
		Velocity * DeltaTime,
		TargetQuat,
		true,
		Hit);

	// 끝점 도달
	const FVector LadderOrigin = ClimbingLadder->GetActorLocation();
	const FVector Diff = UpdatedComponent->GetComponentLocation() - LadderOrigin;
	const float HeightAlong = FVector::DotProduct(Diff, ClimbAxis);
	if (HeightAlong >= ClimbingLadder->GetClimbDistance() - 100.f && ForwardInput > 0.f)
	{
		EndClimbingWith(ELadderExitReason::Top);
		return;
	}

	const float CharHalfZ = CharacterOwner->GetComponentsBoundingBox().GetExtent().Z;
	if (HeightAlong <= 5.f + CharHalfZ && ForwardInput < 0.f)
	{
		EndClimbingWith(ELadderExitReason::Bottom);
		return;
	}
}

void UGYCharacterMovementComponent::StartClimbing(ALadder* Ladder)
{
    if (!Ladder) return;
    ClimbingLadder = Ladder;
    // StopMovementImmediately();
    SetMovementMode(MOVE_Custom, CMOVE_Climbing);
}

void UGYCharacterMovementComponent::StopClimbing()
{
    ClimbingLadder.Reset();
    if (IsClimbing())
    {
        SetMovementMode(MOVE_Falling);
    }
}

bool UGYCharacterMovementComponent::IsClimbing() const
{
    return MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Climbing;
}

void UGYCharacterMovementComponent::EndClimbingWith(ELadderExitReason Reason)
{
    ClimbingLadder.Reset();
    SetMovementMode(MOVE_Falling);

    if (OnClimbingEnded.IsBound())
    {
        OnClimbingEnded.Broadcast(Reason);
    }
}

float UGYCharacterMovementComponent::GetMaxSpeed() const
{
    if (IsClimbing()) return MaxClimbSpeed;
    return Super::GetMaxSpeed();
}

bool UGYCharacterMovementComponent::CanAttemptJump() const
{
    if (IsClimbing()) return false;
    return Super::CanAttemptJump();
}
