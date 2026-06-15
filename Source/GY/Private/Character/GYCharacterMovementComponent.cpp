#include "Character/GYCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "WorldGimmick/Ladder.h"

UGYCharacterMovementComponent::UGYCharacterMovementComponent(const FObjectInitializer& OI)
    : Super(OI)
{
}

void UGYCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
    Super::PhysCustom(DeltaTime, Iterations);
    if (CustomMovementMode == CMOVE_Climbing)
    {
        PhysClimbing(DeltaTime, Iterations);
    }
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
	const FVector InputVec = Acceleration.GetSafeNormal();
	const FVector LocalInput = LadderT.InverseTransformVector(InputVec);

	const float ForwardInput = -LocalInput.X;


	UE_LOG(LogTemp, Warning, TEXT("[Climb] %s Accel=%s LastInput=%s ForwardInput=%.2f"),
		CharacterOwner->HasAuthority() ? TEXT("Server") : TEXT("Client"),
		*Acceleration.ToString(),
		*GetLastInputVector().ToString(),
		ForwardInput);

	// 위/아래 이동
	const FVector ClimbAxis = ClimbingLadder->GetClimbAxis();
	Velocity = ClimbAxis * ForwardInput * MaxClimbSpeed;

	FHitResult Hit;
	SafeMoveUpdatedComponent(
		Velocity * DeltaTime,
		UpdatedComponent->GetComponentQuat(),
		true,
		Hit);

	// 끝점 도달
	const FVector LadderOrigin = ClimbingLadder->GetActorLocation();
	const FVector Diff = UpdatedComponent->GetComponentLocation() - LadderOrigin;
	const float HeightAlong = FVector::DotProduct(Diff, ClimbAxis);

	if (HeightAlong >= ClimbingLadder->GetClimbDistance() && ForwardInput > 0.f)
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
    StopMovementImmediately();
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
