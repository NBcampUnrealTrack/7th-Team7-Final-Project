#include "AbilitySystem/Abilities/Tasks/AbilityTask_DashToTarget.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAbilityTask_DashToTarget* UAbilityTask_DashToTarget::CreateDashToTarget(
    UGameplayAbility* OwningAbility, AActor* Target,
    float InDashSpeed, float InStopDistance, float InFrontHalfAngleDeg)
{
    UAbilityTask_DashToTarget* Task = NewAbilityTask<UAbilityTask_DashToTarget>(OwningAbility);
    Task->TargetActor = Target;
    Task->DashSpeed = FMath::Max(InDashSpeed, 1.f);
    Task->StopDistanceSq = FMath::Square(FMath::Max(InStopDistance, 0.f));
    Task->CosFrontHalfAngle = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(InFrontHalfAngleDeg, 0.f, 180.f)));
    return Task;
}

void UAbilityTask_DashToTarget::Activate()
{
	ACharacter* Char = Cast<ACharacter>(GetAvatarActor());
	if (!Char || !TargetActor.IsValid())
	{
		if (!bBroadcasted) { OnCancelled.Broadcast(); bBroadcasted = true; }
		EndTask();
		return;
	}

	bTickingTask = true;
}

void UAbilityTask_DashToTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	ACharacter* Char = Cast<ACharacter>(GetAvatarActor());
	AActor* Target = TargetActor.Get();
	if (!Char || !Target)
	{
		if (!bBroadcasted) { OnCancelled.Broadcast(); bBroadcasted = true; }
		EndTask();
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - Char->GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.SizeSquared() <= StopDistanceSq)
	{
		if (!bBroadcasted) { OnReachedDistance.Broadcast(); bBroadcasted = true; }
		EndTask();
		return;
	}

	FVector Forward = Char->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	if (!ToTarget.IsNearlyZero())
	{
		const FVector ToTargetDir = ToTarget.GetSafeNormal();
		const float Dot = FVector::DotProduct(Forward, ToTargetDir);
		if (Dot < CosFrontHalfAngle)
		{
			if (!bBroadcasted) { OnOutOfSight.Broadcast(); bBroadcasted = true; }
			EndTask();
			return;
		}
	}

	if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
	{
		CMC->Velocity = Forward * DashSpeed;
	}
}

void UAbilityTask_DashToTarget::OnDestroy(bool bInOwnerFinished)
{
	if (ACharacter* Char = Cast<ACharacter>(GetAvatarActor()))
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			CMC->Velocity = FVector::ZeroVector;
		}
	}
	Super::OnDestroy(bInOwnerFinished);
}
