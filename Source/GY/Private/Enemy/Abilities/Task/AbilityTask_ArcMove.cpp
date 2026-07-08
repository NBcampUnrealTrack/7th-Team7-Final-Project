#include "Enemy/Abilities/Task/AbilityTask_ArcMove.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAbilityTask_ArcMove* UAbilityTask_ArcMove::Create(
	UGameplayAbility* OwningAbility, AActor* InTarget,
	FVector Destination, FVector ControlPoint, float MoveSpeed,
	EArcMoveRotationMode InRotationMode)
{
	UAbilityTask_ArcMove* Task = NewAbilityTask<UAbilityTask_ArcMove>(OwningAbility);
	Task->TargetActor = InTarget;
	Task->EndPos = Destination;
	Task->ControlPos = ControlPoint;
	Task->RotationMode = InRotationMode;

	// Arc length 근사 → Duration 도출
	const float ArcLen = ApproximateBezierLength(
		FVector::ZeroVector, // Activate에서 실제 Start로 재계산
		ControlPoint,
		Destination);
	// Placeholder — Activate에서 Char 위치 알고 재계산
	Task->CachedMoveSpeed = FMath::Max(MoveSpeed, 1.f);
	return Task;
}

FVector UAbilityTask_ArcMove::QuadraticBezier(const FVector& P0, const FVector& P1, const FVector& P2, float T)
{
    const float U = 1.f - T;
    return U * U * P0 + 2.f * U * T * P1 + T * T * P2;
}

float UAbilityTask_ArcMove::ApproximateBezierLength(const FVector& P0, const FVector& P1, const FVector& P2,
	int32 Samples)
{
	float Length = 0.f;
	FVector Prev = P0;
	for (int32 i = 1; i <= Samples; ++i)
	{
		const float T = static_cast<float>(i) / static_cast<float>(Samples);
		const FVector Cur = QuadraticBezier(P0, P1, P2, T);
		Length += FVector::Dist(Prev, Cur);
		Prev = Cur;
	}
	return Length;
}

void UAbilityTask_ArcMove::Activate()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActor());
    if (!Char) { EndTask(); return; }

    StartPos = Char->GetActorLocation();

	const float ArcLen = ApproximateBezierLength(StartPos, ControlPos, EndPos);
	Duration = FMath::Max(ArcLen / CachedMoveSpeed, 0.1f);

    if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
    {
        SavedMovementMode = CMC->MovementMode;
        CMC->SetMovementMode(MOVE_Flying);
        bStateSaved = true;
    }
    bTickingTask = true;
}

void UAbilityTask_ArcMove::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    ACharacter* Char = Cast<ACharacter>(GetAvatarActor());
    if (!Char) { EndTask(); return; }

    Elapsed += DeltaTime;
    const float T = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);

    const FVector NewPos = QuadraticBezier(StartPos, ControlPos, EndPos, T);
    const FVector CurPos = Char->GetActorLocation();
    const FVector Delta = NewPos - CurPos;

    if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
    {
        CMC->Velocity = Delta / FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);
    }

    // Rotation
    switch (RotationMode)
    {
    case EArcMoveRotationMode::FaceMoveDirection:
        if (!Delta.IsNearlyZero())
        {
            FRotator NewRot = Delta.Rotation();
            NewRot.Pitch = 0.f; NewRot.Roll = 0.f;
            Char->SetActorRotation(NewRot);
        }
        break;

    case EArcMoveRotationMode::FaceTarget:
        if (AActor* Focus = TargetActor.Get())
        {
            FVector ToFocus = Focus->GetActorLocation() - CurPos;
            ToFocus.Z = 0.f;
            if (!ToFocus.IsNearlyZero())
            {
                Char->SetActorRotation(ToFocus.Rotation());
            }
        }
        break;

    default: break;
    }

    if (T >= 1.f)
    {
        EndTask();
    }
}

void UAbilityTask_ArcMove::OnDestroy(bool bInOwnerFinished)
{
    if (bStateSaved)
    {
        if (ACharacter* Char = Cast<ACharacter>(GetAvatarActor()))
        {
            if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
            {
                CMC->Velocity = FVector::ZeroVector;
                CMC->SetMovementMode(SavedMovementMode);
            }
        }
        bStateSaved = false;
    }
    OnEnded.Broadcast();
    Super::OnDestroy(bInOwnerFinished);
}
